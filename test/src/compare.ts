import { CommandInterface } from "../../src/emulators";

export interface WaitImageProps {
    resize?: boolean,
    threshold?: number,
    timeout?: number,
    interval?: number,
    maxShift?: number,
    success?: () => Promise<void>;
}

export async function waitImage(imageUrl: string, ci: CommandInterface, options?: WaitImageProps) {
    const threshold = options?.threshold ?? 1;
    const timeout = options?.timeout ?? 3000;
    const interval = options?.interval ?? 64;
    const success = options?.success === undefined ? async () => { } : options.success;
    const resize = options?.resize ?? false;
    const maxShift = options?.maxShift ?? 0;

    let expected: LoadedComparableImage;
    try {
        expected = await loadExpectedImage(imageUrl);
    } catch (e) {
        await ci.exit().catch(() => undefined);
        throw e;
    }

    return new Promise<void>((resolve, reject) => {
        let intervalId = setInterval(() => {
            compare(expected, ci, threshold, false, resize, maxShift)
                .then((error) => {
                    if (intervalId !== null && error === null) {
                        clearInterval(intervalId);
                        intervalId = null;

                        success()
                            .then(() => ci.exit())
                            .then(resolve)
                            .catch(reject);
                    }
                })
                .catch(() => { });
        }, interval);

        setTimeout(() => {
            if (intervalId !== null) {
                clearInterval(intervalId);
                compare(expected, ci, threshold, true, resize, maxShift)
                    .then((error) => {
                        if (error === null) {
                            success()
                                .then(() => ci.exit())
                                .then(resolve)
                                .catch(reject);
                        } else {
                            ci.exit()
                                .then(() => reject(error))
                                .catch(reject);
                        }
                    })
                    .catch(reject);
            }
        }, timeout);
    });
}

const compare = (expected: LoadedComparableImage,
    ci: CommandInterface,
    threshold: number,
    showComparsion: boolean,
    resize: boolean,
    maxShift: number): Promise<null | Error> => {
    if (typeof document === "undefined" || typeof Image === "undefined") {
        return compareNode(expected, ci, threshold, resize, maxShift);
    }

    return compareBrowser(expected, ci, threshold, showComparsion, resize, maxShift);
};

const compareBrowser = (expected: LoadedComparableImage,
    ci: CommandInterface,
    threshold: number,
    showComparsion: boolean,
    resize: boolean,
    maxShift: number): Promise<null | Error> => {
    return ci.screenshot()
        .then(imageDataToUrl)
        .then((actualUrl: string) => new Promise<null | Error>((resolve, reject) => {
            const actualImage = new Image();
            actualImage.onload = () => {
                if (!resize &&
                    (expected.width !== actualImage.width ||
                        expected.height !== actualImage.height)) {
                    if (showComparsion && expected.element !== undefined) {
                        renderComparsion(expected.element, actualImage);
                    }
                    if (expected.width !== actualImage.width) {
                        resolve(new Error("Invalid width: " + actualImage.width + ", should be " + expected.width));
                    } else {
                        resolve(new Error("Invalid height: " + actualImage.height + ", should be " + expected.height));
                    }
                    return;
                }

                const actualCanvas = document.createElement("canvas");
                actualCanvas.width = expected.width;
                actualCanvas.height = expected.height;
                actualCanvas.style.imageRendering = "pixelated";
                const actualCtx = actualCanvas.getContext("2d");
                actualCtx.drawImage(actualImage, 0, 0, expected.width, expected.height);
                const actual = actualCtx.getImageData(0, 0, expected.width, expected.height).data;

                const wrong = comparePixels(expected, {
                    width: expected.width,
                    height: expected.height,
                    data: actual,
                }, maxShift);
                if (showComparsion && wrong > threshold && expected.element !== undefined) {
                    renderComparsion(expected.element, resize ? actualCanvas : actualImage);
                }
                resolve(wrong > threshold ?
                    new Error("Image not same, wrong: " + wrong) :
                    null);
            };
            actualImage.onerror = () => reject(new Error("Failed to load screenshot image"));
            actualImage.src = actualUrl;
        }));
};

interface ComparableImage {
    width: number;
    height: number;
    data: Uint8Array | Uint8ClampedArray;
}

interface LoadedComparableImage extends ComparableImage {
    element?: HTMLImageElement;
}

async function compareNode(expected: LoadedComparableImage,
                           ci: CommandInterface,
                           threshold: number,
                           resize: boolean,
                           maxShift: number): Promise<null | Error> {
    const screenshot = await ci.screenshot();
    let actual: ComparableImage = {
        width: screenshot.width,
        height: screenshot.height,
        data: screenshot.data,
    };

    if (!resize && (expected.width !== actual.width || expected.height !== actual.height)) {
        if (expected.width !== actual.width) {
            return new Error("Invalid width: " + actual.width + ", should be " + expected.width);
        } else {
            return new Error("Invalid height: " + actual.height + ", should be " + expected.height);
        }
    }

    if (resize) {
        actual = resizeNearest(actual, expected.width, expected.height);
    }

    const wrong = comparePixels(expected, actual, maxShift);
    return wrong > threshold ?
        new Error("Image not same, wrong: " + wrong) :
        null;
}

function loadExpectedImage(imageUrl: string): Promise<LoadedComparableImage> {
    if (typeof document === "undefined" || typeof Image === "undefined") {
        return Promise.resolve(readPng(imageUrl));
    }

    return new Promise<LoadedComparableImage>((resolve, reject) => {
        const img = new Image();
        img.onload = () => {
            try {
                const canvas = document.createElement("canvas");
                canvas.width = img.width;
                canvas.height = img.height;
                const ctx = canvas.getContext("2d");
                ctx.drawImage(img, 0, 0);

                resolve({
                    width: img.width,
                    height: img.height,
                    data: ctx.getImageData(0, 0, img.width, img.height).data,
                    element: img,
                });
            } catch (e) {
                reject(e);
            }
        };
        img.onerror = () => reject(new Error("Failed to load image: " + imageUrl));
        img.src = imageUrl;
    });
}

function readPng(imageUrl: string): ComparableImage {
    const load = (module as any)["req" + "uire"].bind(module);
    const fs = load("fs");
    const path = load("path");
    const pngjs = load("pngjs");
    const imagePath = path.isAbsolute(imageUrl) ?
        imageUrl :
        path.resolve(process.cwd(), imageUrl);
    const png = pngjs.PNG.sync.read(fs.readFileSync(imagePath));

    return {
        width: png.width,
        height: png.height,
        data: png.data,
    };
}

function resizeNearest(image: ComparableImage, width: number, height: number): ComparableImage {
    const resized = new Uint8ClampedArray(width * height * 4);

    for (let y = 0; y < height; y++) {
        const sourceY = Math.min(image.height - 1, Math.floor(y * image.height / height));
        for (let x = 0; x < width; x++) {
            const sourceX = Math.min(image.width - 1, Math.floor(x * image.width / width));
            const sourceOffset = (sourceY * image.width + sourceX) * 4;
            const targetOffset = (y * width + x) * 4;

            resized[targetOffset + 0] = image.data[sourceOffset + 0];
            resized[targetOffset + 1] = image.data[sourceOffset + 1];
            resized[targetOffset + 2] = image.data[sourceOffset + 2];
            resized[targetOffset + 3] = image.data[sourceOffset + 3];
        }
    }

    return {
        width,
        height,
        data: resized,
    };
}

function comparePixels(expected: ComparableImage, actual: ComparableImage, maxShift = 0) {
    let best = Number.MAX_SAFE_INTEGER;

    for (let dy = -maxShift; dy <= maxShift; dy++) {
        for (let dx = -maxShift; dx <= maxShift; dx++) {
            best = Math.min(best, comparePixelsAtOffset(expected, actual, dx, dy));
        }
    }

    return best;
}

function comparePixelsAtOffset(expected: ComparableImage, actual: ComparableImage, dx: number, dy: number) {
    let total = 0;
    let compared = 0;

    for (let x = 0; x < expected.width; x++) {
        const actualX = x + dx;
        if (actualX < 0 || actualX >= actual.width) {
            continue;
        }

        for (let y = 0; y < expected.height; y++) {
            const actualY = y + dy;
            if (actualY < 0 || actualY >= actual.height) {
                continue;
            }

            total += Math.abs(expected.data[y * expected.width * 4 + x * 4 + 0] -
                actual.data[actualY * actual.width * 4 + actualX * 4 + 0]);
            total += Math.abs(expected.data[y * expected.width * 4 + x * 4 + 1] -
                actual.data[actualY * actual.width * 4 + actualX * 4 + 1]);
            total += Math.abs(expected.data[y * expected.width * 4 + x * 4 + 2] -
                actual.data[actualY * actual.width * 4 + actualX * 4 + 2]);
            compared += 3;
        }
    }

    return Math.floor(total / compared);
}

function imageDataToUrl(imageData: ImageData) {
    const canvas = document.createElement("canvas");
    canvas.width = imageData.width;
    canvas.height = imageData.height;

    const ctx = canvas.getContext("2d");
    ctx.putImageData(imageData, 0, 0);

    return canvas.toDataURL("image/png");
}

function renderComparsion(img: HTMLImageElement, actualImage: HTMLElement) {
    document.body.appendChild(document.createElement("hr"));
    document.body.appendChild(img); // for comparisons
    const div = document.createElement("div");
    div.innerHTML = "^=expected, v=actual";
    document.body.appendChild(div);
    document.body.appendChild(actualImage); // to grab it for creating the test reference
}

export function renderComparsionOf(a: ImageData, b: ImageData) {
    if (typeof document === "undefined" || typeof Image === "undefined") {
        return;
    }

    const aUrl = imageDataToUrl(a);
    const bUrl = imageDataToUrl(b);
    const aImage = new Image();
    const bImage = new Image();

    aImage.onload = () => {
        bImage.onload = () => {
            renderComparsion(aImage, bImage);
        };
        bImage.src = bUrl;
    };
    aImage.src = aUrl;
}
