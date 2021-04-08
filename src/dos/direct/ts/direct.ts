import { WasmModule } from "../../../impl/modules";
import { TransportLayer, MessageHandler, ClientMessage } from "../../../protocol/protocol";

export async function dosDirect(wasmModule: WasmModule, sessionId: string): Promise<TransportLayer> {
    let handler: MessageHandler = () => {/**/};
    let startupErrorLog: string = "";

    const startupErrFn = (...args: any[]) => {
        console.error(...args);
        startupErrorLog += JSON.stringify(args) + "\n";
    }

    const module: any = {
        err: startupErrFn,
        printErr: startupErrFn,
    };

    const messageEventListener = (e: MessageEvent) => {
        const data = e.data;
        if (data?.name !== undefined) {
            handler(data.name, data.props);
        }
    };

    window.addEventListener("message", messageEventListener, { passive: true });

    const transportLayer: TransportLayer = {
        sessionId,
        sendMessageToServer: (name: ClientMessage, props?: {[key: string]: any}) => {
            postMessage({name, props}, "*");
        },
        setMessageHandler: (newHandler: MessageHandler) => {
            handler = newHandler
        },
        exit: () => {
            window.removeEventListener("message", messageEventListener);
        },
    };

    await wasmModule.instantiate(module);
    if (startupErrorLog.length > 0) {
        transportLayer.sendMessageToServer("wc-exit", {});
        window.removeEventListener("message", messageEventListener);
        throw new Error(startupErrorLog);
    }

    module.err = console.error;
    module.printErr = console.error;

    module.callMain([sessionId]);

    return transportLayer;
}
