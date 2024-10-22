import { CommandInterface, NetworkType, BackendOptions, DosConfig, InitFsEntry, InitFileEntry } from "../emulators";
import { CommandInterfaceEventsImpl } from "../impl/ci-impl";

const maxDataChunkSize = 4 * 1024 * 1024;

export type ClientMessage =
    "wc-install" |
    "wc-run" |
    "wc-pack-fs-to-bundle" |
    "wc-add-key" |
    "wc-mouse-move" |
    "wc-mouse-button" |
    "wc-mouse-sync" |
    "wc-exit" |
    "wc-sync-sleep" |
    "wc-pause" |
    "wc-resume" |
    "wc-mute" |
    "wc-unmute" |
    "wc-connect" |
    "wc-disconnect" |
    "wc-backend-event" |
    "wc-asyncify-stats" |
    "wc-fs-tree" |
    "wc-fs-get-file" |
    "wc-send-data-chunk" |
    "wc-net-connected" |
    "wc-net-received";

export type ServerMessage =
    "ws-extract-progress" |
    "ws-ready" |
    "ws-server-ready" |
    "ws-frame-set-size" |
    "ws-update-lines" |
    "ws-log" |
    "ws-warn" |
    "ws-err" |
    "ws-stdout" |
    "ws-exit" |
    "ws-persist" |
    "ws-sound-init" |
    "ws-sound-push" |
    "ws-config" |
    "ws-sync-sleep" |
    "ws-connected" |
    "ws-disconnected" |
    "ws-asyncify-stats" |
    "ws-fs-tree" |
    "ws-send-data-chunk" |
    "ws-net-connect" |
    "ws-net-disconnect" |
    "ws-net-send";

export type MessageHandler = (name: ServerMessage, props: { [key: string]: any }) => void;

export interface TransportLayer {
    sessionId: string;
    sendMessageToServer(name: ClientMessage,
        props: { [key: string]: any },
        transfer?: ArrayBuffer[]): void;
    initMessageHandler(handler: MessageHandler): void;
    exit?: () => void;
}

export interface FrameLine {
    start: number;
    heapu8: Uint8Array;
}

export interface DataChunk {
    type: "ok" | "file" | "bundle";
    name: string;
    data: ArrayBuffer | null;
}

export interface AsyncifyStats {
    messageSent: number,
    messageReceived: number,
    messageFrame: number,
    messageSound: number,
    nonSkippableSleepCount: number,
    sleepCount: number,
    sleepTime: number,
    cycles: number,
    netSent: number,
    netRecv: number,
    driveSent: number,
    driveRecv: number,
    driveRecvTime: number,
    driveCacheHit: number,
    driveCacheMiss: number,
    driveCacheUsed: number,
    driveBufferedAmount: number,
    driveIo: { read: number, write: number }[];
}

export interface FsNode {
    name: string,
    size: number | null,
    nodes: FsNode[] | null,
}

export class CommandInterfaceOverTransportLayer implements CommandInterface {
    private startedAt = Date.now();
    private exited = false;
    private frameWidth = 0;
    private frameHeight = 0;
    private rgb: Uint8Array | null = null;
    private rgba: Uint8Array | null = null;
    private freq = 0;
    private utf8Decoder = new TextDecoder();

    private init?: InitFsEntry[];
    private transport: TransportLayer;
    private ready: (err: Error | null) => void;

    private persistPromise?: Promise<Uint8Array | null>;
    private persistResolve?: (bundle: Uint8Array | null) => void;

    private exitPromise?: Promise<void>;
    private exitResolve?: () => void;

    private eventsImpl = new CommandInterfaceEventsImpl();

    private keyMatrix: { [keyCode: number]: boolean } = {};

    private configPromise: Promise<DosConfig>;
    private configResolve: (config: DosConfig) => void = () => {/**/};
    private panicMessages: string[] = [];

    private connectPromise: Promise<void> | null = null;
    private connectResolve: () => void = () => {/**/};
    private connectReject: () => void = () => {/**/};

    private disconnectPromise: Promise<void> | null = null;
    private disconnectResolve: () => void = () => {/**/};

    private asyncifyStatsPromise: Promise<AsyncifyStats> | null = null;
    private asyncifyStatsResolve: (stats: AsyncifyStats) => void = () => {/**/};

    private fsTreePromise: Promise<FsNode> | null = null;
    private fsTreeResolve: (fsRoot: FsNode) => void = () => {/**/};

    private fsGetFilePromise: { [name: string]: Promise<Uint8Array> } = {};
    private fsGetFileResolve: { [name: string]: (file: Uint8Array) => void } = {};
    private fsGetFileParts: { [name: string]: Uint8Array[] } = {};

    private dataChunkPromise: { [name: string]: Promise<void> } = {};
    private dataChunkResolve: { [name: string]: () => void } = {};
    private networkId = 0;
    private network: { [id: number]: WebSocket } = {};

    public options: BackendOptions;

    constructor(init: InitFsEntry[],
        transport: TransportLayer,
        ready: (err: Error | null) => void,
        options: BackendOptions) {
        this.options = options;
        this.init = init;
        this.transport = transport;
        this.ready = ready;
        this.configPromise = new Promise<DosConfig>((resolve) => this.configResolve = resolve);
        this.transport.initMessageHandler(this.onServerMessage.bind(this));
    }

    private sendClientMessage(name: ClientMessage, props?: { [key: string]: any }, transfer?: [ArrayBuffer]) {
        props = props || {};
        props.sessionId = props.sessionId || this.transport.sessionId;
        this.transport.sendMessageToServer(name, props, transfer);
    }

    private onServerMessage(name: ServerMessage, props: { [key: string]: any }) {
        if (name === undefined || name.length < 3 ||
            name[0] !== "w" || name[1] !== "s" || name[2] !== "-") {
            return;
        }

        if (props === undefined || props.sessionId !== this.transport.sessionId) {
            return;
        }

        switch (name) {
            case "ws-ready": {
                const sendBundles = async () => {
                    if (!this.init || this.init.length === 0) {
                        return;
                    }

                    const encoder = new TextEncoder();
                    const sendData = async (type: "file" | "bundle", name: string, contents: Uint8Array) => {
                        await this.sendDataChunk({
                            type,
                            name,
                            data: contents.buffer,
                        });

                        await this.sendDataChunk({
                            type,
                            name,
                            data: null,
                        });
                    };

                    let bundleIndex = 0;
                    for (const next of this.init) {
                        if (ArrayBuffer.isView(next)) {
                            await sendData("bundle", bundleIndex + "", next);
                            bundleIndex++;
                        } else if (typeof next === "string") {
                            await sendData("file", ".jsdos/dosbox.conf", encoder.encode(next));
                        } else {
                            const fileEntry = next as InitFileEntry;
                            const dosConfig = next as DosConfig;

                            if (dosConfig.jsdosConf?.version !== undefined) {
                                await sendData("file", ".jsdos/dosbox.conf",
                                    encoder.encode(dosConfig.dosboxConf));
                                await sendData("file", ".jsdos/jsdos.json",
                                    encoder.encode(JSON.stringify(dosConfig.jsdosConf, null, 2)));
                            } else if (fileEntry.path !== undefined) {
                                await sendData("file", fileEntry.path, fileEntry.contents);
                            } else {
                                console.error("Unknown init part", next);
                            }
                        }
                    }
                };

                sendBundles()
                    .then(() => {
                        this.sendClientMessage("wc-run", { token: this.options.token });
                    })
                    .catch((e) => {
                        this.onErr("panic", "Can't send bundles to backend: " + e.message);
                        console.error(e);
                    })
                    .finally(() => {
                        delete this.init;
                    });
            } break;
            case "ws-server-ready": {
                if (this.panicMessages.length > 0) {
                    if (this.transport.exit !== undefined) {
                        this.transport.exit();
                    }
                    this.ready(new Error(JSON.stringify(this.panicMessages)));
                } else {
                    this.ready(null);
                }
                delete (this as any).ready;
            } break;
            case "ws-frame-set-size": {
                this.onFrameSize(props.width, props.height);
            } break;
            case "ws-update-lines": {
                this.onFrameLines(props.lines, props.rgba);
            } break;
            case "ws-exit": {
                this.onExit();
            } break;
            case "ws-log": {
                // eslint-disable-next-line
                this.onLog(props.tag, props.message);
            } break;
            case "ws-warn": {
                // eslint-disable-next-line
                this.onWarn(props.tag, props.message);
            } break;
            case "ws-err": {
                // eslint-disable-next-line
                this.onErr(props.tag, props.message);
            } break;
            case "ws-stdout": {
                this.onStdout(props.message);
            } break;
            case "ws-persist": {
                this.onPersist(props.bundle);
            } break;
            case "ws-sound-init": {
                this.onSoundInit(props.freq);
            } break;
            case "ws-sound-push": {
                this.onSoundPush(props.samples);
            } break;
            case "ws-config": {
                this.onConfig({
                    dosboxConf: this.utf8Decoder.decode(props.dosboxConf),
                    jsdosConf: JSON.parse(props.jsdosConf),
                });
            } break;
            case "ws-sync-sleep": {
                this.sendClientMessage("wc-sync-sleep", props);
            } break;
            case "ws-connected": {
                this.connectResolve();
                this.connectPromise = null;
                this.connectResolve = () => {/**/};
                this.connectReject = () => {/**/};
                this.eventsImpl.fireNetworkConnected(props.networkType, props.address);
            } break;
            case "ws-disconnected": {
                if (this.connectPromise !== null) {
                    this.connectReject();
                    this.connectPromise = null;
                    this.connectResolve = () => {/**/};
                    this.connectReject = () => {/**/};
                } else {
                    this.disconnectResolve();
                    this.disconnectPromise = null;
                    this.disconnectResolve = () => {/**/};
                }
                this.eventsImpl.fireNetworkDisconnected(props.networkType);
            } break;
            case "ws-extract-progress": {
                if (this.options.onExtractProgress) {
                    this.options.onExtractProgress(props.index, props.file, props.extracted, props.count);
                }
            } break;
            case "ws-asyncify-stats": {
                this.asyncifyStatsResolve(props as AsyncifyStats);
                this.asyncifyStatsResolve = () => {/**/};
                this.asyncifyStatsPromise = null;
            } break;
            case "ws-fs-tree": {
                this.fsTreeResolve(props.fsTree as FsNode);
                this.fsTreeResolve = () => {/**/};
                this.fsTreePromise = null;
            } break;
            case "ws-send-data-chunk": {
                const chunk: DataChunk = props.chunk;
                const key = this.dataChunkKey(chunk);
                if (chunk.type === "ok") {
                    if (this.dataChunkPromise[key] !== undefined) {
                        this.dataChunkResolve[key]();
                        delete this.dataChunkPromise[key];
                        delete this.dataChunkResolve[key];
                    }
                } else if (chunk.type === "file") {
                    if (chunk.data === null) {
                        const file = this.mergeChunks(this.fsGetFileParts[chunk.name]);
                        this.fsGetFileResolve[chunk.name](file);
                        delete this.fsGetFilePromise[chunk.name];
                        delete this.fsGetFileResolve[chunk.name];
                    } else {
                        this.fsGetFileParts[chunk.name].push(new Uint8Array(chunk.data));
                    }
                } else {
                    console.log("Unknown chunk type:", chunk.type);
                }
            } break;
            case "ws-net-connect": {
                this.networkId += 1;
                const networkId = this.networkId;
                const socket = new WebSocket(props.address);
                socket.binaryType = "arraybuffer";
                socket.addEventListener("error", (e) => {
                    console.error("Can't connect to", props.address);
                    this.sendClientMessage("wc-net-connected", { networkId: -1 });
                });
                socket.addEventListener("open", () => {
                    this.network[networkId] = socket;
                    this.sendClientMessage("wc-net-connected", { networkId });
                });
                socket.addEventListener("message", (message) => {
                    this.sendClientMessage("wc-net-received", {
                        networkId,
                        data: message.data,
                    }, [message.data]);
                });
            } break;
            case "ws-net-send": {
                const socket = this.network[props.networkId];
                if (socket) {
                    socket.send(props.data);
                }
            } break;
            case "ws-net-disconnect": {
                const socket = this.network[props.networkId];
                delete this.network[props.networkId];
                if (socket) {
                    socket.close();
                }
            } break;
            default: {
                // eslint-disable-next-line
                console.log("Unknown server message (ws):", name);
            } break;
        }
    }

    private onConfig(config: DosConfig) {
        this.configResolve(config);
    }

    private onFrameSize(width: number, height: number) {
        if (this.frameWidth === width && this.frameHeight === height) {
            return;
        }

        this.frameWidth = width;
        this.frameHeight = height;
        this.rgb = new Uint8Array(width * height * 3);
        this.eventsImpl.fireFrameSize(width, height);
    }

    private onFrameLines(lines: FrameLine[], rgbaPtr: number) {
        for (const line of (lines as FrameLine[])) {
            this.rgb!.set(line.heapu8, line.start * this.frameWidth * 3);
        }

        this.eventsImpl.fireFrame(this.rgb, this.rgba);
    }

    private onSoundInit(freq: number) {
        this.freq = freq;
    }

    private onSoundPush(samples: Float32Array) {
        this.eventsImpl.fireSoundPush(samples);
    }

    private onLog(tag: string, message: string) {
        this.eventsImpl.fireMessage("log", "[" + tag + "]" + message);
    }

    private onWarn(tag: string, message: string) {
        this.eventsImpl.fireMessage("warn", "[" + tag + "]" + message);
    }

    private onErr(tag: string, message: string) {
        if (tag === "panic") {
            this.panicMessages.push(message);
            console.error("[" + tag + "]" + message);
        }
        this.eventsImpl.fireMessage("error", "[" + tag + "]" + message);
    }

    private onStdout(message: string) {
        this.eventsImpl.fireStdout(message);
    }

    public config() {
        return this.configPromise;
    }

    public width() {
        return this.frameWidth;
    }

    public height() {
        return this.frameHeight;
    }

    public soundFrequency() {
        return this.freq;
    }

    public screenshot(): Promise<ImageData> {
        if (this.rgb !== null || this.rgba !== null) {
            const rgba = new Uint8ClampedArray(this.frameWidth * this.frameHeight * 4);
            const frame = (this.rgb !== null ? this.rgb : this.rgba) as Uint8Array;

            let frameOffset = 0;
            let rgbaOffset = 0;

            while (rgbaOffset < rgba.length) {
                rgba[rgbaOffset++] = frame[frameOffset++];
                rgba[rgbaOffset++] = frame[frameOffset++];
                rgba[rgbaOffset++] = frame[frameOffset++];
                rgba[rgbaOffset++] = 255;

                if (frame.length === rgba.length) {
                    frameOffset++;
                }
            }

            return Promise.resolve(new ImageData(rgba, this.frameWidth, this.frameHeight));
        } else {
            return Promise.reject(new Error("No frame received"));
        }
    }

    public simulateKeyPress(...keyCodes: number[]) {
        const timeMs = Date.now() - this.startedAt;
        keyCodes.forEach((keyCode) => this.addKey(keyCode, true, timeMs));
        keyCodes.forEach((keyCode) => this.addKey(keyCode, false, timeMs + 16));
    }

    public sendKeyEvent(keyCode: number, pressed: boolean) {
        this.addKey(keyCode, pressed, Date.now() - this.startedAt);
    }

    // public for test
    public addKey(keyCode: number, pressed: boolean, timeMs: number) {
        const keyPressed = this.keyMatrix[keyCode] === true;
        if (keyPressed === pressed) {
            return;
        }
        this.keyMatrix[keyCode] = pressed;
        this.sendClientMessage("wc-add-key", { key: keyCode, pressed, timeMs });
    }

    public sendMouseMotion(x: number, y: number) {
        this.sendClientMessage("wc-mouse-move", { x, y, relative: false, timeMs: Date.now() - this.startedAt });
    }

    public sendMouseRelativeMotion(x: number, y: number) {
        this.sendClientMessage("wc-mouse-move", { x, y, relative: true, timeMs: Date.now() - this.startedAt });
    }

    public sendMouseButton(button: number, pressed: boolean) {
        this.sendClientMessage("wc-mouse-button", { button, pressed, timeMs: Date.now() - this.startedAt });
    }

    public sendMouseSync() {
        this.sendClientMessage("wc-mouse-sync", { timeMs: Date.now() - this.startedAt });
    }

    public sendBackendEvent(payload: any) {
        this.sendClientMessage("wc-backend-event", { json: JSON.stringify(payload) });
    }


    public persist(onlyChanges?: boolean): Promise<Uint8Array | null> {
        if (this.persistPromise !== undefined) {
            return this.persistPromise;
        }


        const persistPromise = new Promise<Uint8Array | null>((resolve) => {
            this.persistResolve = resolve;
        });
        this.persistPromise = persistPromise;
        this.sendClientMessage("wc-pack-fs-to-bundle", {
            onlyChanges: onlyChanges !== false,
        });

        return persistPromise;
    }

    private onPersist(bundle: Uint8Array) {
        if (this.persistResolve) {
            this.persistResolve(bundle);
            delete this.persistPromise;
            delete this.persistResolve;
        }
    }

    public pause() {
        this.sendClientMessage("wc-pause");
    }

    public resume() {
        this.sendClientMessage("wc-resume");
    }

    public mute() {
        this.sendClientMessage("wc-mute");
    }

    public unmute() {
        this.sendClientMessage("wc-unmute");
    }

    public exit(): Promise<void> {
        if (this.exited) {
            return Promise.resolve();
        }
        if (this.exitPromise !== undefined) {
            return this.exitPromise;
        }
        this.exitPromise = new Promise<void>((resolve) => this.exitResolve = resolve);
        this.exitPromise.then(() => {
            this.events().fireExit();
        });

        this.resume();
        for (const next of Object.values(this.network)) {
            next.close();
        }
        this.network = {};
        this.sendClientMessage("wc-exit");

        return this.exitPromise;
    }

    private onExit() {
        if (!this.exited) {
            this.exited = true;
            if (this.transport.exit !== undefined) {
                this.transport.exit();
            }
            if (this.exitResolve) {
                this.exitResolve();
                delete this.exitPromise;
                delete this.exitResolve;
            }
        }
    }

    public events() {
        return this.eventsImpl;
    }

    public networkConnect(networkType: NetworkType, address: string): Promise<void> {
        if (this.connectPromise !== null || this.disconnectPromise !== null) {
            return Promise.reject(new Error("Already prefoming connection or disconnection..."));
        }

        this.connectPromise = new Promise<void>((resolve, reject) => {
            if (!address.startsWith("wss://") && !address.startsWith("ws://")) {
                address = (window.location.protocol === "http:" ? "ws://" : "wss://") + address;
            }

            this.connectResolve = resolve;
            this.connectReject = reject;
            this.sendClientMessage("wc-connect", {
                networkType,
                address,
            });
        });
        return this.connectPromise;
    }

    public networkDisconnect(networkType: NetworkType): Promise<void> {
        if (this.connectPromise !== null || this.disconnectPromise !== null) {
            return Promise.reject(new Error("Already prefoming connection or disconnection..."));
        }

        this.disconnectPromise = new Promise<void>((resolve) => {
            this.disconnectResolve = resolve;

            this.sendClientMessage("wc-disconnect", {
                networkType,
            });
        });
        return this.disconnectPromise;
    }

    public asyncifyStats(): Promise<AsyncifyStats> {
        if (this.asyncifyStatsPromise !== null) {
            return this.asyncifyStatsPromise;
        }

        const promise = new Promise<AsyncifyStats>((resolve) => {
            this.asyncifyStatsResolve = resolve;
        });

        this.asyncifyStatsPromise = promise;
        this.sendClientMessage("wc-asyncify-stats", {});

        return promise;
    }

    public fsTree(): Promise<FsNode> {
        if (this.fsTreePromise !== null) {
            return this.fsTreePromise;
        }

        const promise = new Promise<FsNode>((resolve) => {
            this.fsTreeResolve = resolve;
        });
        this.fsTreePromise = promise;
        this.sendClientMessage("wc-fs-tree");

        return promise;
    }

    async fsReadFile(file: string): Promise<Uint8Array> {
        if (this.fsGetFilePromise[file] !== undefined) {
            throw new Error("fsGetFile should not be called twice for same file");
        }

        const promise = new Promise<Uint8Array>((resolve) => {
            this.fsGetFileResolve[file] = resolve;
        });
        this.fsGetFilePromise[file] = promise;
        this.fsGetFileParts[file] = [];
        this.sendClientMessage("wc-fs-get-file", {
            file,
        });

        return promise;
    }

    async fsWriteFile(file: string, contents: ReadableStream<Uint8Array> | Uint8Array): Promise<void> {
        if (ArrayBuffer.isView(contents)) {
            await this.sendDataChunk({
                type: "file",
                name: file,
                data: contents.buffer,
            });
        } else {
            const reader = contents.getReader();
            while (true) {
                const result = await reader.read();
                if (result.value !== undefined) {
                    await this.sendDataChunk({
                        type: "file",
                        name: file,
                        data: result.value.buffer,
                    });
                }
                if (result.done) {
                    break;
                }
            }
        }

        await this.sendDataChunk({
            type: "file",
            name: file,
            data: null,
        });
    }

    async fsDeleteFile(file: string): Promise<void> {
        throw new Error("not implemented");
    }

    private async sendDataChunk(chunk: DataChunk): Promise<void> {
        if (chunk.data === null || chunk.data.byteLength <= maxDataChunkSize) {
            return this.sendFullDataChunk(chunk);
        } else {
            let pos = 0;
            while (pos < chunk.data.byteLength) {
                await this.sendFullDataChunk({
                    type: chunk.type,
                    name: chunk.name,
                    data: chunk.data.slice(pos, Math.min(chunk.data.byteLength, pos + maxDataChunkSize)),
                });
                pos += maxDataChunkSize;
            }
        }
    }

    private async sendFullDataChunk(chunk: DataChunk): Promise<void> {
        const key = this.dataChunkKey(chunk);
        if (this.dataChunkPromise[key] !== undefined) {
            throw new Error("sendDataChunk should be accepted before sending new one");
        }
        const promise = new Promise<void>((resolve) => {
            this.dataChunkResolve[key] = resolve;
        });
        this.dataChunkPromise[key] = promise;
        this.sendClientMessage("wc-send-data-chunk", {
            chunk,
        }, chunk.data === null ? undefined : [chunk.data]);
        return promise;
    }

    private dataChunkKey(chunk: DataChunk) {
        return chunk.name;
    }

    private mergeChunks(parts: Uint8Array[]): Uint8Array {
        if (parts.length === 1) {
            return parts[0];
        }

        let length = 0;
        for (const next of parts) {
            length += next.byteLength;
        }
        const merged = new Uint8Array(length);
        length = 0;
        for (const next of parts) {
            merged.set(next, length);
            length += next.byteLength;
        }
        return merged;
    }
}
