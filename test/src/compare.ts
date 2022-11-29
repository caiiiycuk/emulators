import { CommandInterface } from "../../src/emulators";

// Compare
// =======
// Compare image from url, and screenshot from DosBox

export function compareAndExit(imageUrl: string, ci: CommandInterface, threshold = 1) {
    return new Promise<void>((resolve, reject) => {
        const fn = () => {
            compare(imageUrl, ci, threshold, 0)
                .then((wrong) => {
                    ci.exit()
                        .then(() => {
                            if (wrong > threshold) {
                                reject(new Error("Image not same, wrong: " + wrong));
                            } else {
                                resolve();
                            }
                        }).catch(reject);
                })
                .catch(reject);
        };

        // give chance to render all queued frames
        setTimeout(fn, 300);
    });
}

export function waitImage(imageUrl: string, ci: CommandInterface, threshold = 1, timeout = 3000) {
    return new Promise<void>((resolve, reject) => {
        let intervalId = setInterval(() => {
            compare(imageUrl, ci, threshold, 0, false)
                .then((wrong) => {
                    if (intervalId !== null && wrong <= threshold) {
                        clearInterval(intervalId);
                        intervalId = null;
                        ci.exit()
                            .then(resolve)
                            .catch(reject);
                    }
                })
                .catch(() => { });
        }, 64);

        setTimeout(() => {
            if (intervalId !== null) {
                clearInterval(intervalId);
                compareAndExit(imageUrl, ci, threshold)
                    .then(resolve)
                    .catch(reject);
            }
        }, timeout);
    });
}

const compare = (imageUrl: string, ci: CommandInterface, threshold: number, delayMs: number, showComparsion = true) => {
    return new Promise((resolve) => setTimeout(resolve, delayMs))
        .then(ci.screenshot.bind(ci))
        .then(imageDataToUrl)
        .then((actualUrl: string) => new Promise<number>((resolve, reject) => {
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
                    if (img.width !== actualImage.width ||
                        img.height !== actualImage.height) {
                        if (showComparsion) {
                            renderComparsion(img, actualImage);
                        }
                        if (img.width !== actualImage.width) {
                            reject(new Error("Invalid width: " + actualImage.width + ", should be " + img.width));
                        } else {
                            reject(new Error("Invalid height: " + actualImage.height + ", should be " + img.height));
                        }
                    }

                    const actualCanvas = document.createElement("canvas");
                    actualCanvas.width = actualImage.width;
                    actualCanvas.height = actualImage.height;
                    const actualCtx = actualCanvas.getContext("2d");
                    actualCtx.drawImage(actualImage, 0, 0);
                    const actual = actualCtx.getImageData(0, 0, actualImage.width, actualImage.height).data;

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
                        renderComparsion(img, actualImage);
                    }
                    resolve(wrong);
                };
                actualImage.src = actualUrl;
            };
            img.src = imageUrl;
        }));
};

function imageDataToUrl(imageData: ImageData) {
    const canvas = document.createElement("canvas");
    canvas.width = imageData.width;
    canvas.height = imageData.height;

    const ctx = canvas.getContext("2d");
    ctx.putImageData(imageData, 0, 0);

    return canvas.toDataURL("image/png");
}

function renderComparsion(img: HTMLImageElement, actualImage: HTMLImageElement) {
    document.body.appendChild(document.createElement("hr"));
    document.body.appendChild(img); // for comparisons
    const div = document.createElement("div");
    div.innerHTML = "^=expected, v=actual";
    document.body.appendChild(div);
    document.body.appendChild(actualImage); // to grab it for creating the test reference
}

export function renderComparsionOf(a: ImageData, b: ImageData) {
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
