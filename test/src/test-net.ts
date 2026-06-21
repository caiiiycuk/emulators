/* eslint-disable new-cap */
import { assert } from "chai";

import DosBundle from "../../src/dos/bundle/dos-bundle";
import { CommandInterface, NetworkType } from "../../src/emulators";
import emulatorsImpl from "../../src/impl/emulators-impl";
import { createNet as createNetImpl, Net } from "../humblenet/humblenet";
import { isJspiSupported } from "./jspi";

type CIFactory = (bundle: Uint8Array | Uint8Array[], net?: Net) => Promise<CommandInterface>;

export function createNet() {
    return createNetImpl("wss://net.js-dos.com:444", "js-dos-test", "1e32rfm", (peerId) => {
        console.log("net: peer unreachable", peerId);
    }, () => {
        console.log("net: disconnected");
    });
};


export function testNet() {
    suite("humblenet");

    test("network should work", async () => {
        const net = await createNet();
        assert.ok(net.peerId !== 0, "Server net peerId is 0");
        net.shutdown();
        return;
    });

    testServer((bundle, net: Net) => emulatorsImpl.dosboxDirect(bundle, { net }), "dosboxDirect", "dosbox");
    testServer((bundle, net: Net) => emulatorsImpl.dosboxWorker(bundle, { net }), "dosboxWorker", "dosbox");
    testServer((bundle, net: Net) => emulatorsImpl.dosboxXDirect(bundle, { net }), "dosboxXDirect", "dosbox-x");
    testServer((bundle, net: Net) => emulatorsImpl.dosboxXWorker(bundle, { net }), "dosboxXWorker", "dosbox-x");
    if (isJspiSupported()) {
        testServer((bundle, net: Net) => emulatorsImpl.dosboxXJspiWorker(bundle, { net }),
            "dosboxXJspiWorker", "dosbox-x-jspi");
    } else {
        console.warn("Skipping dosboxXJspiWorker net tests: JSPI is not supported by this browser.");
    }
}

function testServer(factory: CIFactory, name: string, backend: "dosbox" | "dosbox-x" | "dosbox-x-jspi") {
    async function CI(bundle: DosBundle | Promise<DosBundle>, sharedNet?: Net) {
        const net = sharedNet ?? await createNet();
        bundle = await Promise.resolve(bundle);
        const ci = await factory(await bundle.toUint8Array(), net);
        return {
            ci,
            shutdown: async () => {
                await ci.exit();
                if (!sharedNet) {
                    net.shutdown();
                }
            },
            address: net.peerId.toString(),
        };
    }

    async function createServer(ipxnet: boolean = true, sharedNet?: Net) {
        if (ipxnet) {
            return CI((await emulatorsImpl.bundle()).autoexec("ipxnet startserver"), sharedNet);
        }

        const { ci, shutdown, address } = await CI(await emulatorsImpl.bundle());
        ci.sendBackendEvent({
            type: "wc-trigger-event",
            event: "hand_ipx_startserver",
        });
        return { ci, shutdown, address };
    }

    suite(name + ".ipx");

    async function serverTest({ ci, shutdown }: { ci: CommandInterface, shutdown: () => Promise<void> }) {
        let connected = false;
        let notifiedDisconnected = false;
        const messages: string[] = [];
        assert.ok(ci);
        ci.events().onMessage((mType, message: string) => {
            messages.push(message);
            connected = connected || message.startsWith("[LOG_NET]IPX: Connected to server.  IPX address is");
        });
        ci.events().onNetworkDisconnected(() => {
            notifiedDisconnected = true;
        });
        await sleep(3000);
        await shutdown();

        // assert.ok(notifiedConnected, "Connected is not notified");
        assert.ok(connected, JSON.stringify(messages, null, 2));
        assert.ok(notifiedDisconnected, "Disconnected is not notified");
    };

    test("can create server and connect to self (jsapi)", async () => {
        return serverTest(await createServer(false));
    });

    test("can create server and connect to self (ipxnet)", async () => {
        return serverTest(await createServer());
    });

    test(name + " should not freeze when connecting to wrong address (jsapi)", async () => {
        let notifiedDisconnected = false;
        const messages: string[] = [];
        const { ci, shutdown } = await CI(await emulatorsImpl.bundle());
        assert.ok(ci);
        ci.events().onMessage((mType, message: string) => {
            messages.push(message);
        });
        ci.events().onNetworkDisconnected(() => {
            notifiedDisconnected = true;
        });

        try {
            await ci.networkConnect(NetworkType.NETWORK_DOSBOX_IPX, "1");
            assert.ok(false, JSON.stringify(messages, null, 2));
        } catch (e) {
            assert.ok(notifiedDisconnected, "Disconnected is not notified");
        } finally {
            await shutdown();
        }
    });

    test(name + " client should connect to server (jsapi)", async () => {
        const { address, shutdown: serverShutdown } = await createServer();
        let notifiedConnected = false;
        let notifiedDisconnected = false;
        let connected = false;
        const messages: string[] = [];
        const { ci, shutdown } = await CI(await emulatorsImpl.bundle());
        assert.ok(ci);
        ci.events().onMessage((mType, message: string) => {
            messages.push(message);
            connected = connected || message.startsWith("[LOG_NET]IPX: Connected to server.  IPX address is");
        });
        ci.events().onNetworkConnected(() => {
            notifiedConnected = true;
        });
        ci.events().onNetworkDisconnected(() => {
            notifiedDisconnected = true;
        });

        await sleep(3000);
        await ci.networkConnect(NetworkType.NETWORK_DOSBOX_IPX, address);
        await ci.networkDisconnect(NetworkType.NETWORK_DOSBOX_IPX);
        await serverShutdown();
        await shutdown();

        assert.ok(connected, JSON.stringify(messages, null, 2));
        assert.ok(notifiedConnected, "Connected is not notified");
        assert.ok(notifiedDisconnected, "Disconnected is not notified");
    });

    test(name + " should connect to server (ipxnet)", async () => {
        const { address, shutdown: serverShutdown } = await createServer();
        let notifiedConnected = false;
        let notifiedDisconnected = false;
        let connected = false;
        const messages: string[] = [];
        const { ci, shutdown } = await CI((await emulatorsImpl.bundle())
            .autoexec("ipxnet connect " + address));
        assert.ok(ci);
        ci.events().onMessage((mType, message: string) => {
            messages.push(message);
            connected = connected || message
                .startsWith("[LOG_NET]IPX: Connected to server.  IPX address is");
        });
        ci.events().onNetworkConnected(() => {
            notifiedConnected = true;
        });
        ci.events().onNetworkDisconnected(() => {
            notifiedDisconnected = true;
        });
        await sleep(3000);
        await ci.networkDisconnect(NetworkType.NETWORK_DOSBOX_IPX);
        await serverShutdown();
        await shutdown();

        assert.ok(connected, JSON.stringify(messages, null, 2));
        assert.ok(notifiedConnected, "Connected is not notified");
        assert.ok(notifiedDisconnected, "Disconnected is not notified");
    });


    test(name + " 2 clients can ping each other (ipxnet)", async () => {
        let timeSumMs = 0;
        let timeSamples = 0;
        const portMap: { [port: string]: boolean } = {};
        const regex = new RegExp(/\[LOG_NET\]\d+:.*port\s+(\d+)\s+time=(\d+)ms/);
        const messages: string[] = [];
        const { address, ci: one, shutdown: oneShutdown } = await createServer();
        assert.ok(one);
        await sleep(backend === "dosbox-x" ? 3000 : 300);

        const { ci: two, shutdown: twoShutdown } = await CI((await emulatorsImpl.bundle())
            .autoexec("ipxnet connect " + address + "\nipxnet ping"));
        assert.ok(two);
        two.events().onMessage((mType, message: string) => {
            messages.push(message);
            const groups = regex.exec(message);
            if (groups) {
                console.log(message);
                portMap[groups[1]] = true;
                timeSumMs += Number.parseInt(groups[2]);
                timeSamples++;
            }
        });
        await sleep(backend === "dosbox-x" ? 3000 : 1500);
        await oneShutdown();
        await twoShutdown();

        const usedPorts = Object.keys(portMap);
        assert(usedPorts.length === 1,
            "Should be 1 used port, but have " + JSON.stringify(usedPorts) + ":\n" +
            JSON.stringify(messages, null, 2));

        console.log("PING avg is", Math.round(timeSumMs / timeSamples), "ms");
    });

    test(name + " 2 clients can ping each other (ipxnet) (shared net)", async () => {
        let timeSumMs = 0;
        let timeSamples = 0;
        const portMap: { [port: string]: boolean } = {};
        const regex = new RegExp(/\[LOG_NET\]\d+:.*port\s+(\d+)\s+time=(\d+)ms/);
        const messages: string[] = [];
        const sharedNet = await createNet();


        let sharedAddress;
        {
            const { address, ci: one, shutdown: oneShutdown } = await createServer(true, sharedNet);
            sharedAddress = address;
            assert.ok(one);
            await sleep(backend === "dosbox-x" ? 3000 : 300);
            await oneShutdown();
        }

        const { address, ci: one, shutdown: oneShutdown } = await createServer(true, sharedNet);
        assert.ok(one);
        assert.equal(sharedAddress, address);
        await sleep(backend === "dosbox-x" ? 3000 : 300);

        const { ci: two, shutdown: twoShutdown } = await CI((await emulatorsImpl.bundle())
            .autoexec("ipxnet connect " + address + "\nipxnet ping"));
        assert.ok(two);
        two.events().onMessage((mType, message: string) => {
            messages.push(message);
            const groups = regex.exec(message);
            if (groups) {
                console.log(message);
                portMap[groups[1]] = true;
                timeSumMs += Number.parseInt(groups[2]);
                timeSamples++;
            }
        });
        await sleep(backend === "dosbox-x" ? 3000 : 1500);
        await oneShutdown();
        await twoShutdown();

        const usedPorts = Object.keys(portMap);
        assert(usedPorts.length === 1,
            "Should be 1 used port, but have " + JSON.stringify(usedPorts) + ":\n" +
            JSON.stringify(messages, null, 2));

        console.log("PING avg is", Math.round(timeSumMs / timeSamples), "ms");
        await sharedNet.shutdown();
    });
}

async function sleep(timeMs: number) {
    return new Promise<void>((resolve) => {
        setTimeout(resolve, timeMs);
    });
}
