import { DosConfig } from "../dos/bundle/dos-conf";
import { CommandInterface } from "../emulators";
import { CommandInterfaceEventsImpl } from "../impl/ci-impl";

export type ClientMessage =
    "wc-install" |
    "wc-run" |
    "wc-pack-fs-to-bundle" |
    "wc-add-key" |
    "wc-mouse-move" |
    "wc-mouse-button" |
    "wc-exit" |
    "wc-sync-sleep";

export type ServerMessage =
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
    "ws-sync-sleep";

export type MessageHandler =  (name: ServerMessage, props: {[key: string]: any}) => void;

export interface TransportLayer {
    sessionId: string;
    sendMessageToServer(name: ClientMessage, props?: {[key: string]: any}): void;
    initMessageHandler(handler: MessageHandler): void;
    exit?: () => void;
}

export interface FrameLine {
    start: number;
    heapu8: Uint8Array;
}

export class CommandInterfaceOverTransportLayer implements CommandInterface {
    private startedAt = Date.now();
    private frameWidth: number = 0;
    private frameHeight: number = 0;
    private rgb: Uint8Array = new Uint8Array();
    private freq = 0;

    private bundles?: Uint8Array[];
    private transport: TransportLayer;
    private ready: (err: Error | null) => void;

    private persistPromise?: Promise<Uint8Array>;
    private persistResolve?: (bundle: Uint8Array) => void;

    private exitPromise?: Promise<void>;
    private exitResolve?: () => void;

    private eventsImpl = new CommandInterfaceEventsImpl();

    private keyMatrix: {[keyCode: number]: boolean} = {};

    private configPromise: Promise<DosConfig>;
    private configResolve: (config: DosConfig) => void = () => {/**/};
    private startupErrorLog: string | undefined = "";

    constructor(bundles: Uint8Array[],
                transport: TransportLayer,
                ready: (err: Error | null) => void) {
        this.bundles = bundles;
        this.transport = transport;
        this.ready = ready;
        this.configPromise = new Promise<DosConfig>((resolve) => this.configResolve = resolve);
        this.transport.initMessageHandler(this.onServerMessage.bind(this));
    }

    private sendClientMessage(name: ClientMessage, props?: {[key: string]: any}) {
        props = props || {};
        props.sessionId = props.sessionId || this.transport.sessionId;
        this.transport.sendMessageToServer(name, props);
    }

    private onServerMessage(name: ServerMessage, props: {[key: string]: any}) {
        if (name === undefined || name.length < 3 ||
            name[0] !== "w" || name[1] !== "s" || name[2] !== "-") {
            return;
        }

        if (props === undefined || props.sessionId !== this.transport.sessionId) {
            return;
        }

        switch (name) {
            case "ws-ready": {
                this.sendClientMessage("wc-run", {
                    bundles: this.bundles,
                });
                delete this.bundles;
            } break;
            case "ws-server-ready": {
                if ((this.startupErrorLog || "").length > 0) {
                    if (this.transport.exit !== undefined) {
                        this.transport.exit();
                    }
                    this.ready(new Error(this.startupErrorLog));
                } else {
                    delete this.startupErrorLog;
                    this.ready(null);
                }
                delete this.ready;
            } break;
            case "ws-frame-set-size": {
                this.onFrameSize(props.width, props.height);
            } break;
            case "ws-update-lines": {
                this.onFrameLines(props.lines);
            } break;
            case "ws-exit": {
                this.onExit();
            } break;
            case "ws-log": {
                this.onLog.apply(this, props.args);
            } break;
            case "ws-warn": {
                this.onWarn.apply(this, props.args);
            } break;
            case "ws-err": {
                this.onErr.apply(this, props.args);
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
                this.onConfig(JSON.parse(props.content));
            } break;
            case "ws-sync-sleep": {
                this.sendClientMessage("wc-sync-sleep", props);
            } break;
            default: {
                // tslint:disable-next-line:no-console
                console.log("Unknown server message (ws):", name);
            } break;
        };
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

    private onFrameLines(lines: FrameLine[]) {
        for (const line of lines) {
            this.rgb.set(line.heapu8, line.start * this.frameWidth * 3);
        }
        this.eventsImpl.fireFrame(this.rgb);
    }

    private onSoundInit(freq: number) {
        this.freq = freq;
    }

    private onSoundPush(samples: Float32Array) {
        this.eventsImpl.fireSoundPush(samples);
    }

    private onLog(...args: any[]) {
        this.eventsImpl.fireMessage("log", ...args);
    }

    private onWarn(...args: any[]) {
        this.eventsImpl.fireMessage("warn", ...args);
    }

    private onErr(...args: any[]) {
        this.eventsImpl.fireMessage("error", ...args);
        if (this.startupErrorLog !== undefined) {
            this.startupErrorLog += JSON.stringify(args) + "\n";
        }
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
        const rgba = new Uint8ClampedArray(this.rgb.length / 3 * 4);

        let rgbOffset = 0;
        let rgbaOffset = 0

        while (rgbaOffset < rgba.length) {
            rgba[rgbaOffset++] = this.rgb[rgbOffset++];
            rgba[rgbaOffset++] = this.rgb[rgbOffset++];
            rgba[rgbaOffset++] = this.rgb[rgbOffset++];
            rgba[rgbaOffset++] = 255;
        }

        return Promise.resolve(new ImageData(rgba, this.frameWidth, this.frameHeight));
    }

    public simulateKeyPress(...keyCodes: number[]) {
        const timeMs = Date.now() - this.startedAt;
        keyCodes.forEach(keyCode => this.addKey(keyCode, true, timeMs));
        keyCodes.forEach(keyCode => this.addKey(keyCode, false, timeMs + 16));
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
        this.sendClientMessage("wc-mouse-move", { x, y, timeMs: Date.now() - this.startedAt })
    }

    public sendMouseButton(button: number, pressed: boolean) {
        this.sendClientMessage("wc-mouse-button", { button, pressed, timeMs: Date.now() - this.startedAt });
    }


    public persist(): Promise<Uint8Array> {
        if (this.persistPromise !== undefined) {
            return this.persistPromise;
        }


        const persistPromise = new Promise<Uint8Array>((resolve) => this.persistResolve = resolve);
        this.persistPromise = persistPromise;

        this.sendClientMessage("wc-pack-fs-to-bundle");

        return persistPromise;
    }

    private onPersist(bundle: Uint8Array) {
        if (this.persistResolve) {
            this.persistResolve(bundle);
            delete this.persistPromise;
            delete this.persistResolve;
        }
    }

    public exit(): Promise<void> {
        if (this.exitPromise !== undefined) {
            return this.exitPromise;
        }


        this.exitPromise = new Promise<void>((resolve) => this.exitResolve = resolve);
        this.exitPromise.then(() => {
            this.events().fireExit();
        });

        this.sendClientMessage("wc-exit");

        return this.exitPromise;
    }

    private onExit() {
        if (this.transport.exit !== undefined) {
            this.transport.exit();
        }
        if (this.exitResolve) {
            this.exitResolve();
            delete this.exitPromise;
            delete this.exitResolve;
        }
    }

    public events() {
        return this.eventsImpl;
    }

}
