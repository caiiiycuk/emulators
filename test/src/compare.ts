import { CommandInterface } from "../../src/emulators";

export interface WaitImageProps {
    resize?: boolean,
    threshold?: number,
    timeout?: number,
    success?: () => Promise<void>;
}

export function waitImage(imageUrl: string, ci: CommandInterface, options?: WaitImageProps) {
    const threshold = options?.threshold ?? 1;
    const timeout = options?.timeout ?? 3000;
    const success = options?.success === undefined ? async () => { } : options.success;
    const resize = options?.resize ?? false;

    return new Promise<void>((resolve, reject) => {
        let intervalId = setInterval(() => {
            compare(imageUrl, ci, threshold, false, resize)
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
        }, 64);

        setTimeout(() => {
            if (intervalId !== null) {
                clearInterval(intervalId);
                compare(imageUrl, ci, threshold, true, resize)
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

const compare = (imageUrl: string,
    ci: CommandInterface,
    threshold: number,
    showComparsion: boolean,
    resize: boolean): Promise<null | Error> => {
    if (typeof document === "undefined" || typeof Image === "undefined") {
        return compareNode(imageUrl, ci, threshold, resize);
    }

    return compareBrowser(imageUrl, ci, threshold, showComparsion, resize);
};

const compareBrowser = (imageUrl: string,
    ci: CommandInterface,
    threshold: number,
    showComparsion: boolean,
    resize: boolean): Promise<null | Error> => {
    return ci.screenshot()
        .then(imageDataToUrl)
        .then((actualUrl: string) => new Promise<null | Error>((resolve, reject) => {
            const img = new Image();
            img.onload = () => {
                const canvas = document.createElement("canvas");
                canvas.width = img.width;
                canvas.height = img.height;
                const ctx = canvas.getContext("2d");
                ctx.drawImage(img, 0, 0);
                const expected = ctx.getImageData(0, 0, img.width, img.height).data;

                const actualImage = new Image();
                actualImage.onload = () => {
                    if (!resize &&
                        (img.width !== actualImage.width ||
                            img.height !== actualImage.height)) {
                        if (showComparsion) {
                            renderComparsion(img, actualImage);
                        }
                        if (img.width !== actualImage.width) {
                            resolve(new Error("Invalid width: " + actualImage.width + ", should be " + img.width));
                        } else {
                            resolve(new Error("Invalid height: " + actualImage.height + ", should be " + img.height));
                        }
                    }

                    const actualCanvas = document.createElement("canvas");
                    actualCanvas.width = img.width;
                    actualCanvas.height = img.height;
                    actualCanvas.style.imageRendering = "pixelated";
                    const actualCtx = actualCanvas.getContext("2d");
                    actualCtx.drawImage(actualImage, 0, 0, img.width, img.height);
                    const actual = actualCtx.getImageData(0, 0, img.width, img.height).data;

                    let total = 0;
                    const width = img.width;
                    const height = img.height;
                    for (let x = 0; x < width; x++) {
                        for (let y = 0; y < height; y++) {
                            total += Math.abs(expected[y * width * 4 + x * 4 + 0] - actual[y * width * 4 + x * 4 + 0]);
                            total += Math.abs(expected[y * width * 4 + x * 4 + 1] - actual[y * width * 4 + x * 4 + 1]);
                            total += Math.abs(expected[y * width * 4 + x * 4 + 2] - actual[y * width * 4 + x * 4 + 2]);
                        }
                    }

                    // floor, to allow some margin of error for antialiasing
                    const wrong = Math.floor(total / (img.width * img.height * 3));
                    if (showComparsion && wrong > threshold) {
                        renderComparsion(img, resize ? actualCanvas : actualImage);
                    }
                    resolve(wrong > threshold ?
                        new Error("Image not same, wrong: " + wrong) :
                        null);
                };
                actualImage.src = actualUrl;
            };
            img.src = imageUrl;
        }));
};

interface ComparableImage {
    width: number;
    height: number;
    data: Uint8Array | Uint8ClampedArray;
}

async function compareNode(imageUrl: string,
                           ci: CommandInterface,
                           threshold: number,
                           resize: boolean): Promise<null | Error> {
    const expected = readPng(imageUrl);
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

    const wrong = comparePixels(expected, actual);
    return wrong > threshold ?
        new Error("Image not same, wrong: " + wrong) :
        null;
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

function comparePixels(expected: ComparableImage, actual: ComparableImage) {
    let total = 0;

    for (let x = 0; x < expected.width; x++) {
        for (let y = 0; y < expected.height; y++) {
            total += Math.abs(expected.data[y * expected.width * 4 + x * 4 + 0] -
                actual.data[y * expected.width * 4 + x * 4 + 0]);
            total += Math.abs(expected.data[y * expected.width * 4 + x * 4 + 1] -
                actual.data[y * expected.width * 4 + x * 4 + 1]);
            total += Math.abs(expected.data[y * expected.width * 4 + x * 4 + 2] -
                actual.data[y * expected.width * 4 + x * 4 + 2]);
        }
    }

    return Math.floor(total / (expected.width * expected.height * 3));
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
