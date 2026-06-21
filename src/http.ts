export interface XhrOptions {
    method?: string;
    progress?: (total: number, loaded: number) => void;
    data?: string;
    responseType?: XMLHttpRequestResponseType;
}

export const httpRequest = XhrRequest;

// # XhrRequest
// `XhrRequest` is small wrapper over XMLHttpRequest, that provides some
// handy methods
function XhrRequest(url: string, options: XhrOptions): Promise<string | ArrayBuffer> {
    return new Promise<string | ArrayBuffer>((resolve, reject) => {
        new Xhr(url, {
            ...options,
            success: resolve,
            fail: (message: string) => {
                reject(new Error(message));
            },
        });
    });
}

// private implementation
interface XhrOptionsInternal extends XhrOptions {
    success?: (response: any) => void;
    fail?: (message: string) => void;
}
// * `success` - callback when resource is downloaded
// * `fail` - fail callback


// Class Xhr does not have any public methods
class Xhr {
    private resource: string;
    private options: XhrOptionsInternal;
    private xhr: XMLHttpRequest | null = null;
    private total = 0;
    private loaded = 0;

    constructor(url: string, options: XhrOptionsInternal) {
        this.resource = url;
        this.options = options;
        this.options.method = options.method || "GET";

        if (this.options.method !== "GET") {
            throw new Error("Method " + this.options.method + " is not supported");
        }

        if (typeof XMLHttpRequest === "undefined") {
            this.makeNodeRequest();
        } else {
            this.makeHttpRequest();
        }
    }

    private makeNodeRequest() {
        const load = (module as any)["req" + "uire"].bind(module);
        const fs = load("fs");
        const path = load("path");
        const url = load("url");

        let resourcePath = this.resource;
        if (resourcePath.startsWith("file://")) {
            resourcePath = url.fileURLToPath(resourcePath);
        } else if (resourcePath.startsWith("/")) {
            const absolutePath = resourcePath;
            const cwdPath = path.resolve(process.cwd(), "." + resourcePath);
            resourcePath = fs.existsSync(absolutePath) ? absolutePath : cwdPath;
        } else {
            resourcePath = path.resolve(process.cwd(), resourcePath);
        }

        fs.readFile(resourcePath, (err: any, buffer: any) => {
            if (err !== null) {
                if (this.options.fail) {
                    this.options.fail("Unable to download '" + this.resource + "', code: 404");
                    delete this.options.fail;
                }
                return;
            }

            if (this.options.progress) {
                this.options.progress(buffer.byteLength, buffer.byteLength);
            }

            if (this.options.success) {
                if (this.options.responseType === "arraybuffer") {
                    const arrayBuffer = buffer.buffer
                        .slice(buffer.byteOffset, buffer.byteOffset + buffer.byteLength);
                    this.options.success(arrayBuffer);
                } else {
                    this.options.success(buffer.toString("utf8"));
                }
            }
        });
    }

    private makeHttpRequest() {
        this.xhr = new XMLHttpRequest();
        this.xhr.open(this.options.method || "GET", this.resource, true);
        if (this.options.method === "POST") {
            this.xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        }
        this.xhr.overrideMimeType("text/plain; charset=x-user-defined");

        let progressListner;
        if (typeof (progressListner = this.xhr).addEventListener === "function") {
            progressListner.addEventListener("progress", (evt) => {
                this.total = evt.total;
                this.loaded = evt.loaded;
                if (this.options.progress) {
                    return this.options.progress(evt.total, evt.loaded);
                }
            });
        }

        let errorListener;
        if (typeof (errorListener = this.xhr).addEventListener === "function") {
            errorListener.addEventListener("error", () => {
                if (this.options.fail) {
                    this.options.fail("Unalbe to download '" + this.resource +
                        "', code: " + (this.xhr as XMLHttpRequest).status);
                    return delete this.options.fail;
                }
            });
        }
        this.xhr.onreadystatechange = () => {
            return this.onReadyStateChange();
        };
        if (this.options.responseType) {
            this.xhr.responseType = this.options.responseType;
        }
        this.xhr.send(this.options.data);
    }

    private onReadyStateChange() {
        const xhr = (this.xhr as XMLHttpRequest);
        if (xhr.readyState === 4) {
            if (xhr.status === 200) {
                if (this.options.success) {
                    const total = Math.max(this.total, this.loaded);
                    if (this.options.progress !== undefined) {
                        this.options.progress(total, total);
                    }

                    return this.options.success(xhr.response);
                }
            } else if (this.options.fail) {
                this.options.fail("Unable to download '" + this.resource + "', code: " + xhr.status);
                return delete this.options.fail;
            }
        }
    }
}
