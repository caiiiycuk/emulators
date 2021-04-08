import { WasmModule } from "../../../impl/modules";
import { TransportLayer, MessageHandler, ClientMessage } from "../../../protocol/protocol";

export async function dosWorker(workerUrl: string,
                                 wasmModule: WasmModule,
                                 sessionId: string): Promise<TransportLayer> {
    let handler: MessageHandler = () => { /**/ };
    const worker = new Worker(workerUrl);
    worker.onerror = (e) => {
        handler("ws-err", { type: e.type, filename: e.filename, message: e.message });
    };
    worker.onmessage = (e) => {
        const data = e.data;
        if (data?.name !== undefined) {
            handler(data.name, data.props);
        }
    }

    await wasmModule.instantiate({});

    const transportLayer: TransportLayer = {
        sessionId,
        sendMessageToServer: (name: ClientMessage, props?: {[key: string]: any}) => {
            worker.postMessage({ name, props });
        },
        setMessageHandler: (newHandler: MessageHandler) => {
            handler = newHandler;
        },
        exit: () => {
            worker.terminate();
        },
    };

    try {
        transportLayer.sendMessageToServer("wc-install", {
            module: (wasmModule as any).wasmModule,
            sessionId,
        });
    } catch (e) {
        transportLayer.sendMessageToServer("wc-install", { sessionId });
    }

    return transportLayer;
}
