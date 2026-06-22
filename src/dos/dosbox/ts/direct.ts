import { WasmModule } from "../../../impl/modules";
import { TransportLayer, MessageHandler, ClientMessage, ServerMessage, Net } from "../../../protocol/protocol";
import { MessagesQueue } from "../../../protocol/messages-queue";
import { createAudioPort } from "./audio-worklet";
// Raw Node wasm wrappers expect sockdrive helpers to exist on the global object.
import "../../../sockdrive/sockdrive";

export async function dosDirect(wasmModule: WasmModule,
                                sessionId: string,
                                canvas?: OffscreenCanvas,
                                audioWorklet?: boolean,
                                net?: Net): Promise<TransportLayer> {
    const messagesQueue = new MessagesQueue();
    let handler: MessageHandler = messagesQueue.handler.bind(messagesQueue);

    const module: any = {};

    module.postMessage = (name: ServerMessage, props: { [key: string]: any }) => {
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
        sendMessageToServer: (name: ClientMessage, props?: { [key: string]: any }) => {
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
        net: net ?? null,
    };

    (transportLayer as any).module = module;

    if (typeof window !== "undefined") {
        window.addEventListener("message", sleepHandler, { passive: true });
    }

    module.canvas = canvas;
    if (audioWorklet) {
        module.audioPort = await createAudioPort();
    }
    await wasmModule.instantiate(module);
    module.callMain([sessionId]);

    return transportLayer;
}
