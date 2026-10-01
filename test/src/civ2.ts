import { join } from "path";
import { Page } from "playwright";
import { PNG } from "pngjs";

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

function hasWindowsDesktop(png: PNG) {
    const desktop = fraction(png, 0, 0, 640, 450,
        (r, g, b) => r < 20 && g > 90 && g < 170 && b > 90 && b < 170 && Math.abs(g - b) < 35);
    const taskbar = fraction(png, 0, 455, 640, 25,
        (r, g, b) => r > 150 && r < 230 && Math.abs(r - g) < 20 && Math.abs(g - b) < 20);

    return desktop > 0.55 && taskbar > 0.45;
}

function hasGameAfterDesktop(png: PNG) {
    return !hasWindowsDesktop(png) && fraction(png, 0, 0, 640, 480,
        (r, g, b) => r > 25 || g > 25 || b > 25) > 0.2;
}

export async function runCiv2(page: Page, artifactsDir: string, signal: AbortSignal) {
    const started = Date.now();
    const deadline = started + Number(process.env.CIV2_TEST_TIMEOUT_MS ?? 5 * 60 * 1000);
    const canvas = page.locator("canvas");
    const checkAbort = () => {
        if (signal.aborted) {
            throw new Error("Civilization II diagnostic cancelled");
        }
    };
    const capture = async (name: string) => {
        checkAbort();
        return await canvas.screenshot({ path: join(artifactsDir, name + ".png"), timeout: 5000 });
    };
    const read = async (name: string) => {
        let screenshot;
        try {
            screenshot = await capture(name);
        } catch (error) {
            const message = String((error as Error).message ?? error).split("\n")[0];
            console.log("Civilization II screenshot skipped: " + message);
            return undefined;
        }
        const png = PNG.sync.read(screenshot);
        if (png.width !== 640 || png.height !== 480) {
            throw new Error("Unexpected canvas screenshot size: " + png.width + "x" + png.height);
        }
        return png;
    };

    await Promise.race([
        page.evaluate(() => (window as any).ready),
        page.waitForTimeout(Math.max(1, deadline - Date.now())).then(() => {
            throw new Error("Timed out waiting for Civilization II emulator start");
        }),
    ]);

    let frame = 0;
    let desktopSeen = false;
    while (Date.now() < deadline) {
        const png = await read("desktop-waiting");
        if (png === undefined) {
            await page.waitForTimeout(1000);
            continue;
        }
        if (frame++ < 12) {
            await capture("boot-" + frame);
        }
        if (hasWindowsDesktop(png)) {
            await capture("desktop");
            console.log("Civilization II desktop at " + (Date.now() - started) + "ms");
            desktopSeen = true;
            break;
        }
        await page.waitForTimeout(1000);
    }
    if (!desktopSeen) {
        throw new Error("Timed out waiting for Windows desktop");
    }

    let matchingFrames = 0;
    while (Date.now() < deadline) {
        const png = await read("game-waiting");
        if (png === undefined) {
            await page.waitForTimeout(1000);
            continue;
        }
        if (hasGameAfterDesktop(png)) {
            matchingFrames++;
            if (matchingFrames >= 3) {
                await capture("game");
                console.log("Civilization II left desktop at " + (Date.now() - started) + "ms");
                return;
            }
        } else {
            matchingFrames = 0;
        }
        await page.waitForTimeout(1000);
    }

    throw new Error("Timed out waiting for Civilization II to leave the Windows desktop");
}
