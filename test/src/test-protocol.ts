import { assert } from "chai";

import {
    CommandInterfaceOverTransportLayer,
    ClientMessage,
    MessageHandler,
    Net,
    ServerMessage,
    TransportLayer,
} from "../../src/protocol/protocol";
import { BackendOptions } from "../../src/emulators";

class TestTransportLayer implements TransportLayer {
    sessionId = "test-session";
    net: Net | null = null;
    handler: MessageHandler = () => {/**/};
    messages: { name: ClientMessage, props: { [key: string]: any } }[] = [];

    sendMessageToServer(name: ClientMessage,
        props: { [key: string]: any },
        transfer?: Transferable[]) {
        this.messages.push({ name, props });
    }

    initMessageHandler(handler: MessageHandler) {
        this.handler = handler;
    }

    sendMessageToClient(name: ServerMessage, props: { [key: string]: any }) {
        this.handler(name, {
            sessionId: this.sessionId,
            ...props,
        });
    }
}

export function testProtocol() {
    suite("protocol");

    test("drops stale frame lines after frame size shrink", () => {
        const { ci, transport } = createCommandInterface();
        let frameCount = 0;

        ci.events().onFrame(() => frameCount++);
        transport.sendMessageToClient("ws-frame-set-size", { width: 640, height: 480 });
        transport.sendMessageToClient("ws-frame-set-size", { width: 320, height: 240 });

        assert.doesNotThrow(() => transport.sendMessageToClient("ws-update-lines", {
            width: 640,
            height: 480,
            lines: [
                { start: 0, heapu8: new Uint8Array(640 * 480 * 3) },
            ],
        }));
        assert.equal(frameCount, 0);
        assert.equal(ci.width(), 320);
        assert.equal(ci.height(), 240);
    });

    test("accepts frame lines that match current frame size", () => {
        const { ci, transport } = createCommandInterface();
        const expected = new Uint8Array([
            1, 2, 3, 4, 5, 6,
            7, 8, 9, 10, 11, 12,
        ]);
        let actual: Uint8Array | null = null;

        ci.events().onFrame((rgb) => actual = rgb);
        transport.sendMessageToClient("ws-frame-set-size", { width: 2, height: 2 });
        transport.sendMessageToClient("ws-update-lines", {
            width: 2,
            height: 2,
            lines: [
                { start: 0, heapu8: expected },
            ],
        });

        assert.ok(actual);
        assert.deepEqual(Array.from(actual!), Array.from(expected));
    });

    test("passes optional MCP server port to wc-run", async () => {
        const { transport } = createCommandInterface({ mcpServerPort: 58992 });

        transport.sendMessageToClient("ws-ready", {});
        await new Promise((resolve) => setTimeout(resolve, 0));

        const run = transport.messages.find((message) => message.name === "wc-run");
        assert.ok(run);
        assert.equal(run!.props.mcpServerPort, 58992);
    });
}

function createCommandInterface(options: BackendOptions = {}) {
    const transport = new TestTransportLayer();
    const ci = new CommandInterfaceOverTransportLayer([], transport, (err) => {
        if (err !== null) {
            throw err;
        }
    }, options);

    return { ci, transport };
}
