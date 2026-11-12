import { assert } from "chai";
import { Browser } from "playwright";

export async function runAudioLifecycle(browser: Browser, baseUrl: string) {
    for (const backend of ["dosboxDirect", "dosboxWorker", "dosboxXDirect", "dosboxXWorker"]) {
        const page = await browser.newPage();
        const errors: string[] = [];
        page.on("pageerror", (error) => errors.push(error.message));
        try {
            await page.route(baseUrl + "/test/test.html", (route) => route.fulfill({
                contentType: "text/html", body: "<!doctype html><script src='/emulators.js'></script>",
            }));
            await page.addInitScript(() => {
                const contexts: AudioContext[] = [];
                const nodes: AudioWorkletNode[] = [];
                const workers: Worker[] = [];
                const NativeAudioContext = window.AudioContext;
                const NativeAudioWorkletNode = window.AudioWorkletNode;
                const NativeWorker = window.Worker;
                (window as any).audioLifecycle = { contexts, nodes, workers, startedWithoutGesture: false };
                window.AudioContext = class extends NativeAudioContext {
                    resumeCount = 0;
                    constructor(options?: AudioContextOptions) {
                        super(options);
                        contexts.push(this);
                        (window as any).audioLifecycle.startedWithoutGesture =
                            !(navigator as any).userActivation.hasBeenActive;
                    }
                    resume() {
                        this.resumeCount++;
                        return super.resume();
                    }
                };
                window.AudioWorkletNode = class extends NativeAudioWorkletNode {
                    disconnectCount = 0;
                    constructor(context: BaseAudioContext, name: string, options?: AudioWorkletNodeOptions) {
                        super(context, name, options);
                        nodes.push(this);
                    }
                    disconnect() {
                        this.disconnectCount++;
                        super.disconnect();
                    }
                };
                window.Worker = class extends NativeWorker {
                    terminateCount = 0;
                    constructor(scriptURL: string | URL, options?: WorkerOptions) {
                        super(scriptURL, options);
                        workers.push(this);
                    }
                    terminate() {
                        this.terminateCount++;
                        super.terminate();
                    }
                };
            });
            await page.goto(baseUrl + "/test/test.html", { waitUntil: "load" });
            const session = await page.context().newCDPSession(page);
            for (let index = 0; index < 3; ++index) {
                // SAFETY: Playwright evaluate grants user activation and would skip the suspended-start case.
                const creation = await session.send("Runtime.evaluate", {
                    expression: `(async () => {
                        emulators.pathPrefix = "/";
                        audioLifecycle.ci = await emulators[${JSON.stringify(backend)}](
                            "[autoexec]\\n@echo off", { audioWorklet: true });
                    })()`,
                    awaitPromise: true, userGesture: false,
                });
                assert.isUndefined(creation.exceptionDetails, JSON.stringify(creation.exceptionDetails));
                const states = await page.evaluate(() => {
                    const state = (window as any).audioLifecycle;
                    return {
                        contexts: state.contexts.map((context: AudioContext) => context.state),
                        nodes: state.nodes.length,
                        startedWithoutGesture: state.startedWithoutGesture,
                    };
                });
                assert.lengthOf(states.contexts, index + 1, backend + ": native context missing");
                assert.equal(states.nodes, index + 1, backend + ": native worklet missing");
                assert.deepEqual(states.contexts.slice(0, index), new Array(index).fill("closed"));
                if (index === 0) {
                    assert.isTrue(states.startedWithoutGesture, backend + ": test must start before user activation");
                    assert.equal(states.contexts[0], "suspended", backend + ": test must start before a gesture");
                } else {
                    await page.mouse.click(300, 300);
                    await page.waitForFunction(() => {
                        const state = (window as any).audioLifecycle;
                        return state.contexts[state.contexts.length - 1].state === "running";
                    }, undefined, { timeout: 10000 });
                }
                if (index === 1) {
                    const afterError = await page.evaluate((workerBackend) => {
                        const state = (window as any).audioLifecycle;
                        const messages: { type: string, message: string }[] = [];
                        state.ci.events().onMessage((type: string, message: string) =>
                            messages.push({ type, message }));
                        messages.length = 0;
                        const props = { sessionId: state.ci.transport.sessionId,
                            tag: "fixture", message: "fixture recoverable backend error" };
                        const worker = state.workers[state.workers.length - 1];
                        if (workerBackend) {
                            worker.dispatchEvent(new MessageEvent("message", { data: { name: "ws-err", props } }));
                            worker.dispatchEvent(new ErrorEvent("error", {
                                message: "fixture recoverable worker error",
                            }));
                        } else {
                            state.ci.transport.module.postMessage("ws-err", props);
                        }
                        return {
                            messages,
                            contexts: state.contexts.map((context: AudioContext) => context.state),
                            disconnects: state.nodes.map((node: any) => node.disconnectCount),
                            terminated: workerBackend ? worker.terminateCount : 0,
                        };
                    }, backend.endsWith("Worker"));
                    assert.deepEqual(afterError.messages, [
                        { type: "error", message: "[fixture]fixture recoverable backend error" },
                        ...(backend.endsWith("Worker") ? [
                            { type: "error", message: "[worker]fixture recoverable worker error" },
                        ] : []),
                    ], backend + ": ws-err was not delivered");
                    assert.deepEqual(afterError.contexts, ["closed", "running"], backend + ": error closed audio");
                    assert.deepEqual(afterError.disconnects, [1, 0], backend + ": error disconnected audio");
                    assert.equal(afterError.terminated, 0, backend + ": error terminated the worker before ws-exit");
                }
                await page.evaluate(async () => {
                    await (window as any).audioLifecycle.ci.exit();
                    (window as any).audioLifecycle.ci = undefined;
                });
                await page.waitForFunction(() => (window as any).audioLifecycle.contexts.every(
                    (context: AudioContext) => context.state === "closed"), undefined, { timeout: 10000 });
                const resumeCounts = await page.evaluate(() => (window as any).audioLifecycle.contexts.map(
                    (context: any) => context.resumeCount));
                await page.mouse.click(300, 300);
                await page.keyboard.press("Space");
                assert.deepEqual(await page.evaluate(() => (window as any).audioLifecycle.contexts.map(
                    (context: any) => context.resumeCount)), resumeCounts, backend + ": stopped context resumed");
                assert.deepEqual(await page.evaluate(() => (window as any).audioLifecycle.nodes.map(
                    (node: any) => node.disconnectCount)), new Array(index + 1).fill(1));
            }
            assert.deepEqual(errors, [], backend + ": browser errors");
            console.log("Audio lifecycle passed: " + backend + " (ws-err, suspended stop, running stop, remount).");
        } finally {
            await page.close();
        }
    }
}
