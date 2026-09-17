import { existsSync, readFileSync } from "fs";
import { join } from "path";
import { Page } from "playwright";
import { PNG } from "pngjs";
import {
    colorHistogram, histogramSimilarity, meanHistogram, readHistogramReference,
    referenceHistogram, smoothHistogram, writeHistogramReference,
} from "./histogram";

const referencePath = join(__dirname, "..", "dosbox-x", "d3d_tunnel.hist.json");
const tunnelSamples = 20;
// Everything except the yellow fps overlay on top and the mode line at the bottom.
const tunnelRegion = { x: 0, y: 40, width: 640, height: 400 };
const histogramBins = 16;
// Healthy frames score >= ~0.6 against the reference (with the same GPU), a shift
// of every channel by 16 levels still scores ~0.55, black is ~0.23, white is 0.
const minHistogramSimilarity = 0.4;
// The demo briefly renders malformed geometry when its camera spline wraps.
// At one-second sampling this can affect one or two frames per run.
const maxCycleTransitionSamples = 2;

function fraction(png: PNG, x: number, y: number, width: number, height: number,
                  matches: (r: number, g: number, b: number) => boolean) {
    let count = 0;
    for (let row = y; row < y + height; row++) {
        for (let col = x; col < x + width; col++) {
            const offset = (row * png.width + col) * 4;
            if (matches(png.data[offset], png.data[offset + 1], png.data[offset + 2])) {
                count++;
            }
        }
    }
    return count / (width * height);
}

function hasFpsOverlay(png: PNG) {
    return fraction(png, 200, 0, 210, 28,
        (r, g, b) => r > 180 && g > 180 && b < 80) > 0.03;
}

function whiteCoverage(png: PNG) {
    return fraction(png, 0, 40, 640, 400,
        (r, g, b) => r > 220 && g > 220 && b > 220);
}

function tunnelCoverage(png: PNG) {
    return fraction(png, 0, 40, 640, 400,
        (r, g, b) => r > 20 && r < 110 && g > 20 && g < 110 && b > 20 && b < 130);
}

function sampleName(sample: number) {
    return "tunnel-" + sample.toString().padStart(2, "0");
}

function tunnelHistogram(png: PNG) {
    return smoothHistogram(colorHistogram(png, tunnelRegion, histogramBins), histogramBins);
}

export async function runD3DTunnel(page: Page, artifactsDir: string, signal: AbortSignal) {
    const started = Date.now();
    const reference = referenceHistogram(readHistogramReference(referencePath));
    const checkAbort = () => {
        if (signal.aborted) {
            throw new Error("D3DTunnel diagnostic cancelled");
        }
    };
    const capture = async (name: string) => {
        checkAbort();
        return await page.screenshot({ path: join(artifactsDir, name + ".png"), timeout: 5000 });
    };

    await page.evaluate(() => (window as any).ready);

    const deadline = Date.now() + 180000;
    let frame = 0;
    while (Date.now() < deadline) {
        const png = PNG.sync.read(await capture("waiting"));
        if (png.width !== 640 || png.height !== 480) {
            throw new Error("Unexpected canvas screenshot size: " + png.width + "x" + png.height);
        }
        if (frame++ < 12) {
            await capture("boot-" + frame);
        }
        if (hasFpsOverlay(png)) {
            let maxWhite = 0;
            let minTextured = 1;
            let minSimilarity = 1;
            let leastSimilarFrame = "";
            const transitionFrames: string[] = [];
            for (let sample = 1; sample <= tunnelSamples; sample++) {
                await page.waitForTimeout(1000);
                const name = sampleName(sample);
                const tunnel = PNG.sync.read(await capture(name));
                const white = whiteCoverage(tunnel);
                const textured = tunnelCoverage(tunnel);
                const similarity = histogramSimilarity(tunnelHistogram(tunnel), reference);
                maxWhite = Math.max(maxWhite, white);
                minTextured = Math.min(minTextured, textured);
                if (similarity < minSimilarity) {
                    minSimilarity = similarity;
                    leastSimilarFrame = name;
                }
                if (textured < 0.2 || similarity < minHistogramSimilarity) {
                    transitionFrames.push(name);
                }
                console.log("D3DTunnel sample " + sample + " at " + (Date.now() - started) +
                    "ms, white=" + white.toFixed(3) + ", texture=" + textured.toFixed(3) +
                    ", histogram=" + similarity.toFixed(3));
                if (white > 0.5) {
                    throw new Error("D3DTunnel rendered a white screen in " + name +
                        " (coverage " + white.toFixed(3) + ")");
                }
            }
            console.log("D3DTunnel max white=" + maxWhite.toFixed(3) +
                ", min texture=" + minTextured.toFixed(3) +
                ", min histogram similarity=" + minSimilarity.toFixed(3) + " at " + leastSimilarFrame);
            if (transitionFrames.length > maxCycleTransitionSamples) {
                throw new Error("D3DTunnel rendered " + transitionFrames.length +
                    " unhealthy samples: " + transitionFrames.join(", ") +
                    " (minimum texture coverage " + minTextured.toFixed(3) +
                    ", minimum histogram similarity " + minSimilarity.toFixed(3) +
                    " at " + leastSimilarFrame + ", reference " + referencePath + ")");
            }
            if (transitionFrames.length > 0) {
                console.log("D3DTunnel ignored " + transitionFrames.length +
                    " camera spline transition sample(s): " + transitionFrames.join(", "));
            }
            return;
        }
        await page.waitForTimeout(1000);
    }
    throw new Error("Timed out waiting for D3DTunnel");
}

// Rebuild the reference histogram from the tunnel-NN.png frames of a known good run:
//   node -r ts-node/register/transpile-only test/src/d3dtunnel.ts --reference dist/test-artifacts/d3dtunnel
function writeReference(artifactsDir: string) {
    const histograms: Float64Array[] = [];
    for (let sample = 1; sample <= tunnelSamples; sample++) {
        const path = join(artifactsDir, sampleName(sample) + ".png");
        if (!existsSync(path)) {
            throw new Error("Missing " + path);
        }
        histograms.push(colorHistogram(PNG.sync.read(readFileSync(path)), tunnelRegion, histogramBins));
    }
    writeHistogramReference(referencePath, meanHistogram(histograms), histogramBins, tunnelRegion, histograms.length);
    const reference = referenceHistogram(readHistogramReference(referencePath));
    const similarities = histograms.map((histogram) =>
        histogramSimilarity(smoothHistogram(histogram, histogramBins), reference));
    console.log("Wrote " + referencePath + " from " + histograms.length + " frames, similarity of the frames: " +
        Math.min(...similarities).toFixed(3) + ".." + Math.max(...similarities).toFixed(3));
}

if (require.main === module) {
    const index = process.argv.indexOf("--reference");
    if (index === -1 || process.argv[index + 1] === undefined) {
        console.error("Usage: d3dtunnel.ts --reference <artifacts dir with tunnel-NN.png>");
        process.exit(1);
    }
    writeReference(process.argv[index + 1]);
}
