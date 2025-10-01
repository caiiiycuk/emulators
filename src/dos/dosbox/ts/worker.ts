import { WasmModule } from "../../../impl/modules";
import { TransportLayer, MessageHandler, ClientMessage, Net } from "../../../protocol/protocol";
import { MessagesQueue } from "../../../protocol/messages-queue";
import { createAudioPort } from "./audio-worklet";

export async function dosWorker(workerUrl: string,
                                wasmModule: WasmModule,
                                sessionId: string,
                                canvas?: OffscreenCanvas,
                                audioWorklet?: boolean,
                                net?: Net): Promise<TransportLayer> {
    const messagesQueue = new MessagesQueue();
    let handler: MessageHandler = messagesQueue.handler.bind(messagesQueue);

    const response = await fetch(workerUrl);
    if (response.status !== 200) {
        throw new Error("Unable to download '" + workerUrl + "' (" +
            response.status + "): " + response.statusText);
    }
    const localUrl = URL.createObjectURL(await response.blob());
    const worker = new Worker(localUrl);
    worker.onerror = (e) => {
        handler("ws-err", { type: e.type, filename: e.filename, message: e.message });
    };
    worker.onmessage = (e) => {
        const data = e.data;
        if (data?.name !== undefined) {
            handler(data.name, data.props);
        }
    };

    const transportLayer: TransportLayer = {
        sessionId,
        sendMessageToServer: (name: ClientMessage,
            props: {[key: string]: any},
            transfer?: ArrayBuffer[]) => {
            if (transfer) {
                worker.postMessage({ name, props }, transfer);
            } else {
                worker.postMessage({ name, props });
            }
        },
        initMessageHandler: (newHandler: MessageHandler) => {
            handler = newHandler;
            messagesQueue.sendTo(handler);
        },
        exit: () => {
            URL.revokeObjectURL(localUrl);
            worker.terminate();
        },
        net: net ?? null,
    };

    const transfer: Transferable[] = canvas ? [canvas] : [];
    let audioPort;

    if (audioWorklet) {
        audioPort = await createAudioPort();
        if (audioPort) {
            transfer.push(audioPort);
        }
    }

    try {
        transportLayer.sendMessageToServer("wc-install", {
            module: (wasmModule as any).wasmModule,
            sessionId,
            canvas,
            audioPort,
        }, transfer);
    } catch (e) {
        transportLayer.sendMessageToServer("wc-install", {
            sessionId,
            canvas,
            audioPort,
        }, transfer);
    }

    return transportLayer;
}
