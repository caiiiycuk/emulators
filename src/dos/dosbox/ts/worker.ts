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
    let audio: Awaited<ReturnType<typeof createAudioPort>>;
    worker.onerror = (e) => {
        handler("ws-err", {
            sessionId, tag: "worker", message: e.message + (e.filename ? " (" + e.filename + ")" : ""),
        });
    };
    worker.onmessage = (e) => {
        const data = e.data;
        if (data?.name !== undefined) {
            handler(data.name, data.props);
        } else if (data?.type === "MIDI Raw Byte" || data?.type === "MIDI_BYTE") {
            // Direct output to the browser console on the Main Thread side!
            /*
            console.log(
                "[WadNexus MIDI Byte]:",
                data.data.toString(16).toUpperCase(),
                "(Hex:", data.data, "res:", String.fromCharCode(data.data), ")",
            );
            */
            // Passing the MIDI byte via a handler to the main thread.
            window.dispatchEvent(new CustomEvent("mt32-midi-byte", { detail: data.data }));
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
            audio?.dispose();
            URL.revokeObjectURL(localUrl);
            worker.terminate();
        },
        net: net ?? null,
    };

    const transfer: Transferable[] = canvas ? [canvas] : [];
    try {
        if (audioWorklet) {
            audio = await createAudioPort();
            if (audio !== undefined) {
                transfer.push(audio.port);
            }
        }
        try {
            transportLayer.sendMessageToServer("wc-install", {
                module: (wasmModule as any).wasmModule,
                sessionId,
                canvas,
                audioPort: audio?.port,
            }, transfer);
        } catch (e) {
            transportLayer.sendMessageToServer("wc-install", {
                sessionId,
                canvas,
                audioPort: audio?.port,
            }, transfer);
        }
        return transportLayer;
    } catch (error) {
        transportLayer.exit?.();
        throw error;
    }
}
