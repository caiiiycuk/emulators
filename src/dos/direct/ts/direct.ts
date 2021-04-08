import { WasmModule } from "../../../impl/modules";
import { TransportLayer, MessageHandler, ClientMessage, ServerMessage } from "../../../protocol/protocol";
import { MessagesQueue } from "../../../protocol/messages-queue";

export async function dosDirect(wasmModule: WasmModule, sessionId: string): Promise<TransportLayer> {
    const messagesQueue = new MessagesQueue();
    let handler: MessageHandler = messagesQueue.handler.bind(messagesQueue);
    let startupErrorLog: string = "";

    const startupErrFn = (...args: any[]) => {
        console.error(...args);
        startupErrorLog += JSON.stringify(args) + "\n";
    }

    const module: any = {
        err: startupErrFn,
        printErr: startupErrFn,
    };

    module.postMessage = (name: ServerMessage, props: {[key: string]: any}) => {
        handler(name, props);
    };

    const sleepHandler = (e: MessageEvent) => {
        const data = e.data;
        if (data?.name === "ws-sync-sleep" && data.props.sessionId === sessionId) {
            postMessage({ name: "wc-sync-sleep", props: data.props }, "*");
        }
    };

    const transportLayer: TransportLayer = {
        sessionId,
        sendMessageToServer: (name: ClientMessage, props?: {[key: string]: any}) => {
            module.messageHandler({ data: { name, props } });
        },
        initMessageHandler: (newHandler: MessageHandler) => {
            handler = newHandler;
            messagesQueue.sendTo(handler);
        },
        exit: () => {
            if (typeof window !== "undefined") {
                window.removeEventListener("message", sleepHandler);
            }
        },
    };

    if (typeof window !== "undefined") {
        window.addEventListener("message", sleepHandler, { passive: true });
    }

    await wasmModule.instantiate(module);
    if (startupErrorLog.length > 0) {
        transportLayer.sendMessageToServer("wc-exit", {});
        throw new Error(startupErrorLog);
    }

    module.err = console.error;
    module.printErr = console.error;

    module.callMain([sessionId]);

    return transportLayer;
}
