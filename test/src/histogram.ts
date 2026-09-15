import { readFileSync, writeFileSync } from "fs";
import { PNG } from "pngjs";

export interface Region {
    x: number;
    y: number;
    width: number;
    height: number;
}

export interface HistogramReference {
    bins: number;
    region: Region;
    frames: number;
    // sparse normalized histogram: [bin index, fraction of pixels]
    histogram: [number, number][];
}

// Normalized 3D RGB histogram of a region, bins x bins x bins cells.
export function colorHistogram(png: PNG, region: Region, bins: number): Float64Array {
    const histogram = new Float64Array(bins * bins * bins);
    const scale = 256 / bins;
    for (let row = region.y; row < region.y + region.height; row++) {
        for (let col = region.x; col < region.x + region.width; col++) {
            const offset = (row * png.width + col) * 4;
            const r = Math.floor(png.data[offset] / scale);
            const g = Math.floor(png.data[offset + 1] / scale);
            const b = Math.floor(png.data[offset + 2] / scale);
            histogram[(r * bins + g) * bins + b]++;
        }
    }
    const total = region.width * region.height;
    for (let i = 0; i < histogram.length; i++) {
        histogram[i] /= total;
    }
    return histogram;
}

// Box blur over the 3x3x3 neighbourhood, so a small colour shift between GPUs
// (rounding, dithering, different texture filtering) does not move pixels
// into disjoint cells.
export function smoothHistogram(histogram: Float64Array, bins: number): Float64Array {
    const smoothed = new Float64Array(histogram.length);
    for (let r = 0; r < bins; r++) {
        for (let g = 0; g < bins; g++) {
            for (let b = 0; b < bins; b++) {
                let sum = 0;
                let count = 0;
                for (let dr = -1; dr <= 1; dr++) {
                    for (let dg = -1; dg <= 1; dg++) {
                        for (let db = -1; db <= 1; db++) {
                            const rr = r + dr;
                            const gg = g + dg;
                            const bb = b + db;
                            if (rr < 0 || gg < 0 || bb < 0 || rr >= bins || gg >= bins || bb >= bins) {
                                continue;
                            }
                            sum += histogram[(rr * bins + gg) * bins + bb];
                            count++;
                        }
                    }
                }
                smoothed[(r * bins + g) * bins + b] = sum / count;
            }
        }
    }
    let total = 0;
    for (let i = 0; i < smoothed.length; i++) {
        total += smoothed[i];
    }
    for (let i = 0; i < smoothed.length; i++) {
        smoothed[i] /= total;
    }
    return smoothed;
}

// Histogram intersection: 1 for identical distributions, 0 for disjoint ones.
export function histogramSimilarity(a: Float64Array, b: Float64Array): number {
    let similarity = 0;
    for (let i = 0; i < a.length; i++) {
        similarity += Math.min(a[i], b[i]);
    }
    return similarity;
}

export function meanHistogram(histograms: Float64Array[]): Float64Array {
    const mean = new Float64Array(histograms[0].length);
    for (const histogram of histograms) {
        for (let i = 0; i < histogram.length; i++) {
            mean[i] += histogram[i] / histograms.length;
        }
    }
    return mean;
}

export function writeHistogramReference(path: string, histogram: Float64Array,
                                        bins: number, region: Region, frames: number) {
    const sparse: [number, number][] = [];
    for (let i = 0; i < histogram.length; i++) {
        if (histogram[i] > 0) {
            sparse.push([i, Number(histogram[i].toFixed(6))]);
        }
    }
    const reference: HistogramReference = { bins, region, frames, histogram: sparse };
    // one [index, value] pair per line keeps the file diffable
    const json = JSON.stringify(reference, null, 2)
        .replace(/\[\s+(\d+),\s+([\d.e-]+)\s+\]/g, "[$1, $2]");
    writeFileSync(path, json + "\n");
}

export function readHistogramReference(path: string): HistogramReference {
    return JSON.parse(readFileSync(path, "utf8")) as HistogramReference;
}

export function referenceHistogram(reference: HistogramReference): Float64Array {
    const histogram = new Float64Array(reference.bins ** 3);
    for (const [index, value] of reference.histogram) {
        histogram[index] = value;
    }
    return smoothHistogram(histogram, reference.bins);
}
