import { appendFileSync } from "fs";
import { join } from "path";
import { Page } from "playwright";
import { PNG } from "pngjs";

// Screen cues, not rendering baselines: title lettering, open passport, and health bar.
// These deliberately do not compare the damaged level textures to a reference image.
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

function hasTitle(png: PNG) {
    return fraction(png, 130, 28, 350, 75, (r, g, b) => r > 90 && r > g * 1.4 && g > b * 1.3) > 0.3;
}

function hasPassport(png: PNG) {
    return hasTitle(png) && fraction(png, 190, 230, 100, 130,
        (r, g, b) => r > 40 && r < 130 && Math.abs(r - g) < 30 && Math.abs(g - b) < 30) > 0.9;
}

function hasLevel(png: PNG) {
    return !hasTitle(png) && fraction(png, 8, 8, 99, 4,
        (r, g, b) => r > 60 && r > g && g > b * 1.3) > 0.65;
}

function assertSnowTexture(png: PNG, name: string) {
    // The initial room has a grey snow floor to Lara's right. The stale texture
    // cache rendered this whole area dark teal. Ignore Lara and the moving camera.
    const snow = fraction(png, 400, 340, 200, 100,
        (r, g, b) => r > 30 && Math.abs(r - g) < 10 && Math.abs(g - b) < 10);
    if (snow < 0.75) {
        throw new Error(name + ": missing snow texture (grey floor coverage " + snow.toFixed(3) + ")");
    }
}

export async function runTomb3dfx(page: Page, artifactsDir: string, signal: AbortSignal) {
    const started = Date.now();
    const canvas = page.locator("canvas");
    const checkAbort = () => {
        if (signal.aborted) {
            throw new Error("Tomb Raider diagnostic cancelled");
        }
    };
    const capture = async (name: string) => {
        checkAbort();
        return await canvas.screenshot({ path: join(artifactsDir, name + ".png"), timeout: 5000 });
    };
    const waitForScreen = async (name: string, matches: (png: PNG) => boolean, timeoutMs: number) => {
        const deadline = Date.now() + timeoutMs;
        let frame = 0;
        while (Date.now() < deadline) {
            const png = PNG.sync.read(await capture(name + "-waiting"));
            if (png.width !== 640 || png.height !== 480) {
                throw new Error("Unexpected canvas screenshot size: " + png.width + "x" + png.height);
            }
            if (matches(png)) {
                await capture(name);
                console.log("Tomb Raider screen: " + name + " at " + (Date.now() - started) + "ms");
                return;
            }
            // Retain a few boot frames as well as the most recent frame for each stage.
            if (name === "menu" && frame++ < 8) {
                await capture("boot-" + frame);
            }
            await page.waitForTimeout(200);
        }
        throw new Error("Timed out waiting for Tomb Raider screen: " + name);
    };
    const key = async (name: string, code: number) => {
        checkAbort();
        const send = async (pressed: boolean) => {
            appendFileSync(join(artifactsDir, "keys.jsonl"), JSON.stringify({
                ms: Date.now() - started, key: name, code, pressed,
            }) + "\n");
            await page.evaluate(({ code, pressed }) => (window as any).ci.sendKeyEvent(code, pressed),
                { code, pressed });
        };
        try {
            await send(true);
            await page.waitForTimeout(100);
        } finally {
            await send(false);
        }
        await page.waitForTimeout(250);
    };

    await page.evaluate(() => (window as any).ready);
    await waitForScreen("menu", hasTitle, 60000);
    // Allow the initial inventory ring to finish opening, well before the idle demo.
    await page.waitForTimeout(800);
    await key("Enter", 257);
    await waitForScreen("passport", hasPassport, 10000);
    await page.waitForTimeout(800);
    await key("Enter", 257);
    await waitForScreen("level", hasLevel, 30000);
    // Let the introductory camera settle. No movement keys are sent.
    await page.waitForTimeout(2000);
    const before = PNG.sync.read(await capture("level-before-f4"));
    await key("F4", 293);
    await page.waitForTimeout(700);
    await capture("level-between-f4");
    await key("F4", 293);
    await page.waitForTimeout(1000);
    const after = PNG.sync.read(await capture("level-after-f4"));
    // Preserve both diagnostic images even when the regression check fails.
    assertSnowTexture(before, "level-before-f4");
    assertSnowTexture(after, "level-after-f4");
}
