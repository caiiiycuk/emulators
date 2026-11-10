import { assert } from "chai";
import { createAudioPort } from "../../src/dos/dosbox/ts/audio-worklet";
import { dosDirect } from "../../src/dos/dosbox/ts/direct";
import { dosWorker } from "../../src/dos/dosbox/ts/worker";
import { CommandInterfaceOverTransportLayer } from "../../src/protocol/protocol";

export function testAudioLifecycle() {
    suite("audio lifecycle");

    for (const backend of ["direct", "worker"]) {
        for (const fail of [false, true]) {
            test(`${backend} releases audio after ${fail ? "initialization failure" : "exit"}`, async () => {
                const browser = createAudioBrowser();
                browser.failInstall = fail;
                try {
                    const module = {
                        instantiate: async (instance: any) => {
                            if (fail) {
                                throw new Error("fixture initialization failure");
                            }
                            instance.callMain = () => undefined;
                        },
                    } as any;
                    const creating = backend === "worker" ?
                        dosWorker("data:text/javascript,", module, "fixture", undefined, true) :
                        dosDirect(module, "fixture", undefined, true);
                    if (fail) {
                        try {
                            await creating;
                            assert.fail("initialization should fail");
                        } catch (error) {
                            assert.include(String(error), "fixture initialization failure");
                        }
                    } else {
                        const transport = await creating;
                        transport.exit?.();
                        transport.exit?.();
                    }
                    assert.deepEqual(browser.contexts.map((context) => context.closeCount), [1]);
                    assert.deepEqual(browser.nodes.map((node) => node.closedPorts), [1]);
                    assert.deepEqual(browser.nodes.map((node) => node.disconnected), [1]);
                    assert.equal(browser.listeners.get("pointerdown")?.size ?? 0, 0);
                    assert.equal(browser.listeners.get("keydown")?.size ?? 0, 0);
                    assert.equal(browser.listeners.get("message")?.size ?? 0, 0);
                } finally {
                    browser.restore();
                }
            });
        }
    }

    for (const failure of ["module", "node"]) {
        test(`closes the context when ${failure} creation fails`, async () => {
            const browser = createAudioBrowser();
            browser.failModule = failure === "module";
            browser.failNode = failure === "node";
            try {
                assert.isUndefined(await createAudioPort());
                assert.deepEqual(browser.contexts.map((context) => context.closeCount), [1]);
                assert.equal(browser.listeners.size, 0);
            } finally {
                browser.restore();
            }
        });
    }

    for (const backend of ["direct", "worker"]) {
        test(`${backend} preserves audio on ws-err and completes CommandInterface exit`, async () => {
            const browser = createAudioBrowser();
            try {
                const transport = backend === "worker" ?
                    await dosWorker("data:text/javascript,", {} as any, "fixture", undefined, true) :
                    await dosDirect({ instantiate: async (instance: any) => {
                        instance.callMain = () => undefined;
                        instance.messageHandler = ({ data }: any) => {
                            if (data.name === "wc-exit") {
                                queueMicrotask(() => instance.postMessage("ws-exit", data.props));
                            }
                        };
                    } } as any, "fixture", undefined, true);
                const ci = new CommandInterfaceOverTransportLayer([], transport, () => undefined, {});
                let exitCount = 0;
                ci.events().onExit(() => exitCount++);
                const messages: string[] = [];
                ci.events().onMessage((type, message) => messages.push(type + ":" + message));
                const postMessage = backend === "worker" ?
                    browser.workerMessage : (transport as any).module.postMessage;
                postMessage("ws-server-ready", { sessionId: "fixture" });
                for (const tag of ["fixture", "panic"]) {
                    postMessage("ws-err", { sessionId: "fixture", tag, message: "fixture backend error" });
                }
                assert.deepEqual(messages, [
                    "error:[fixture]fixture backend error", "error:[panic]fixture backend error",
                ]);
                assert.deepEqual(browser.contexts.map((context) => context.closeCount), [0]);
                assert.deepEqual(browser.nodes.map((node) => node.closedPorts), [0]);
                assert.deepEqual(browser.nodes.map((node) => node.disconnected), [0]);
                assert.equal(browser.listeners.get("pointerdown")?.size, 1);
                assert.equal(browser.listeners.get("keydown")?.size, 1);
                const exiting = ci.exit();
                assert.strictEqual(ci.exit(), exiting);
                assert.deepEqual(browser.contexts.map((context) => context.closeCount), [0]);
                await exiting;
                await ci.exit();
                assert.equal(exitCount, 1);
                assert.deepEqual(browser.contexts.map((context) => context.closeCount), [1]);
                assert.deepEqual(browser.nodes.map((node) => node.closedPorts), [1]);
                assert.deepEqual(browser.nodes.map((node) => node.disconnected), [1]);
                assert.equal(browser.listeners.get("message")?.size ?? 0, 0);
            } finally {
                browser.restore();
            }
        });
    }

    for (const audioWorklet of [false, true]) {
        test(`worker errors preserve CommandInterface exit with audioWorklet=${audioWorklet}`, async () => {
            const browser = createAudioBrowser();
            try {
                const transport = await dosWorker("data:text/javascript,", {} as any, "fixture",
                    undefined, audioWorklet);
                const ci = new CommandInterfaceOverTransportLayer([], transport, () => undefined, {});
                let exitCount = 0;
                ci.events().onExit(() => exitCount++);
                browser.workerError();
                assert.equal(browser.terminatedWorkers, 0);
                assert.deepEqual(browser.contexts.map((context) => context.closeCount), audioWorklet ? [0] : []);
                await ci.exit();
                await ci.exit();
                assert.equal(exitCount, 1);
                assert.equal(browser.terminatedWorkers, 1);
                assert.deepEqual(browser.contexts.map((context) => context.closeCount), audioWorklet ? [1] : []);
            } finally {
                browser.restore();
            }
        });
    }

    test("direct releases audio when callMain fails", async () => {
        const browser = createAudioBrowser();
        const failure = new Error("fixture callMain failure");
        try {
            try {
                await dosDirect({ instantiate: async (instance: any) => {
                    instance.callMain = () => {
                        throw failure;
                    };
                } } as any, "fixture", undefined, true);
                assert.fail("callMain should fail");
            } catch (error) {
                assert.strictEqual(error, failure);
            }
            assert.deepEqual(browser.contexts.map((context) => context.closeCount), [1]);
            assert.deepEqual(browser.nodes.map((node) => node.closedPorts), [1]);
            assert.deepEqual(browser.nodes.map((node) => node.disconnected), [1]);
            assert.equal(browser.listeners.get("pointerdown")?.size ?? 0, 0);
            assert.equal(browser.listeners.get("keydown")?.size ?? 0, 0);
            assert.equal(browser.listeners.get("message")?.size ?? 0, 0);
        } finally {
            browser.restore();
        }
    });

    test("worker retries installation without the compiled module", async () => {
        const browser = createAudioBrowser();
        browser.failFirstInstall = true;
        const module = {} as any;
        try {
            const transport = await dosWorker("data:text/javascript,", { wasmModule: module } as any,
                "fixture", undefined, true);
            assert.lengthOf(browser.installations, 2);
            assert.strictEqual(browser.installations[0].props.module, module);
            assert.isUndefined(browser.installations[1].props.module);
            assert.strictEqual(browser.installations[0].props.audioPort, browser.installations[1].props.audioPort);
            transport.exit?.();
            assert.deepEqual(browser.contexts.map((context) => context.closeCount), [1]);
        } finally {
            browser.restore();
        }
    });

    for (const backend of ["direct", "worker"]) {
        test(`${backend} preserves the fallback when audio creation fails`, async () => {
            const browser = createAudioBrowser();
            browser.failModule = true;
            try {
                const transport = backend === "worker" ?
                    await dosWorker("data:text/javascript,", {} as any, "fixture", undefined, true) :
                    await dosDirect({ instantiate: async (instance: any) => {
                        assert.isUndefined(instance.audioPort);
                        instance.callMain = () => undefined;
                    } } as any, "fixture", undefined, true);
                if (backend === "worker") {
                    assert.isUndefined(browser.installations[0].props.audioPort);
                }
                transport.exit?.();
                assert.deepEqual(browser.contexts.map((context) => context.closeCount), [1]);
                assert.lengthOf(browser.nodes, 0);
            } finally {
                browser.restore();
            }
        });
    }

    test("worker errors during audio creation preserve the late context and installation", async () => {
        const browser = createAudioBrowser();
        browser.failDuringModule = true;
        try {
            const transport = await dosWorker("data:text/javascript,", {} as any, "fixture", undefined, true);
            assert.deepEqual(browser.contexts.map((context) => context.closeCount), [0]);
            assert.deepEqual(browser.nodes.map((node) => node.closedPorts), [0]);
            assert.deepEqual(browser.nodes.map((node) => node.disconnected), [0]);
            assert.lengthOf(browser.installations, 1);
            assert.equal(browser.terminatedWorkers, 0);
            assert.equal(browser.listeners.get("pointerdown")?.size, 1);
            assert.equal(browser.listeners.get("keydown")?.size, 1);
            transport.exit?.();
            assert.deepEqual(browser.contexts.map((context) => context.closeCount), [1]);
            assert.deepEqual(browser.nodes.map((node) => node.closedPorts), [1]);
            assert.deepEqual(browser.nodes.map((node) => node.disconnected), [1]);
            assert.equal(browser.terminatedWorkers, 1);
            assert.equal(browser.listeners.get("pointerdown")?.size ?? 0, 0);
            assert.equal(browser.listeners.get("keydown")?.size ?? 0, 0);
        } finally {
            browser.restore();
        }
    });

    test("worker errors preserve the live audio resources and error message", async () => {
        const browser = createAudioBrowser();
        try {
            const transport = await dosWorker("data:text/javascript,", {} as any, "fixture", undefined, true);
            const messages: { name: string, props: any }[] = [];
            transport.initMessageHandler((name, props) => messages.push({ name, props }));
            browser.workerError();
            assert.deepEqual(messages, [{ name: "ws-err", props: {
                type: "error", filename: undefined, message: "fixture worker failure",
            } }]);
            assert.deepEqual(browser.contexts.map((context) => context.closeCount), [0]);
            assert.deepEqual(browser.nodes.map((node) => node.closedPorts), [0]);
            assert.deepEqual(browser.nodes.map((node) => node.disconnected), [0]);
            assert.equal(browser.listeners.get("pointerdown")?.size, 1);
            assert.equal(browser.listeners.get("keydown")?.size, 1);
            transport.exit?.();
            assert.deepEqual(browser.contexts.map((context) => context.closeCount), [1]);
            assert.deepEqual(browser.nodes.map((node) => node.closedPorts), [1]);
            assert.equal(browser.listeners.get("pointerdown")?.size ?? 0, 0);
            assert.equal(browser.listeners.get("keydown")?.size ?? 0, 0);
        } finally {
            browser.restore();
        }
    });
}

function createAudioBrowser() {
    const browser = {
        contexts: [] as { state: string, closeCount: number }[],
        nodes: [] as { closedPorts: number, disconnected: number }[],
        listeners: new Map<string, Set<() => void>>(),
        failModule: false,
        failDuringModule: false,
        failNode: false,
        failInstall: false,
        failFirstInstall: false,
        installations: [] as { props: { module?: unknown, audioPort?: unknown } }[],
        terminatedWorkers: 0,
        workerError: () => undefined as void,
        workerMessage: (_name: string, _props: any) => undefined as void,
        restore: () => undefined as void,
    };
    class Context {
        state = "suspended";
        closeCount = 0;
        destination = {};
        audioWorklet = {
            addModule: async () => {
                if (browser.failDuringModule) {
                    browser.workerError();
                }
                if (browser.failModule) {
                    throw new Error("fixture module failure");
                }
            },
        };
        constructor() {
            browser.contexts.push(this);
        }
        close() {
            this.closeCount++;
            this.state = "closed";
            return Promise.resolve();
        }
        resume() {
            this.state = "running";
            return Promise.resolve();
        }
    }
    class Node {
        closedPorts = 0;
        disconnected = 0;
        port = { close: () => this.closedPorts++ };
        constructor() {
            if (browser.failNode) {
                throw new Error("fixture node failure");
            }
            browser.nodes.push(this);
        }
        connect() {
            return undefined;
        }
        disconnect() {
            this.disconnected++;
        }
    }
    class Worker {
        onerror: ((event: any) => void) | null = null;
        onmessage: ((event: any) => void) | null = null;
        terminated = false;
        constructor() {
            browser.workerError = () => this.onerror?.({ type: "error", message: "fixture worker failure" });
            browser.workerMessage = (name, props) => this.onmessage?.({ data: { name, props } });
        }
        postMessage(message: { name: string, props: any }) {
            if (this.terminated) {
                return;
            }
            if (message.name === "wc-install") {
                browser.installations.push(message);
                if (browser.failInstall || browser.failFirstInstall && browser.installations.length === 1) {
                    throw new Error("fixture initialization failure");
                }
            } else if (message.name === "wc-exit") {
                queueMicrotask(() => this.onmessage?.({ data: { name: "ws-exit", props: message.props } }));
            }
        }
        terminate() {
            this.terminated = true;
            browser.terminatedWorkers++;
        }
    }
    const eventTarget = {
        addEventListener: (name: string, callback: () => void) => {
            if (!browser.listeners.has(name)) {
                browser.listeners.set(name, new Set());
            }
            browser.listeners.get(name)?.add(callback);
        },
        removeEventListener: (name: string, callback: () => void) => {
            browser.listeners.get(name)?.delete(callback);
        },
    };
    const replacements = {
        AudioContext: Context,
        AudioWorkletNode: Node,
        Worker,
        document: eventTarget,
        window: eventTarget,
    };
    const descriptors = Object.keys(replacements).map((name) => [name,
        Object.getOwnPropertyDescriptor(globalThis, name)] as const);
    for (const [name, value] of Object.entries(replacements)) {
        Object.defineProperty(globalThis, name, { configurable: true, writable: true, value });
    }
    browser.restore = () => {
        for (const [name, descriptor] of descriptors) {
            if (descriptor !== undefined) {
                Object.defineProperty(globalThis, name, descriptor);
            } else {
                delete (globalThis as any)[name];
            }
        }
    };
    return browser;
}
