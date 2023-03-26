/* eslint-disable new-cap */
import { assert } from "chai";

import DosBundle from "../../src/dos/bundle/dos-bundle";
import { CommandInterface, NetworkType } from "../../src/emulators";
import emulatorsImpl from "../../src/impl/emulators-impl";

type CIFactory = (bundle: Uint8Array | Uint8Array[]) => Promise<CommandInterface>;

const defaultIpxServerAddress = "127.0.0.1";
const defaultIpxServerPort = 1900;
const room = "test_" + Math.round(Math.random() * 100000);
const wsPrefix = (window.location.protocol === "http:" ? "ws://" : "wss://");

export function testNet() {
    testServer((bundle) => emulatorsImpl.dosboxDirect(bundle), "dosboxDirect", "dosbox");
    testServer((bundle) => emulatorsImpl.dosboxWorker(bundle), "dosboxWorker", "dosbox");
    testServer((bundle) => emulatorsImpl.dosboxXDirect(bundle), "dosboxXDirect", "dosbox-x");
    testServer((bundle) => emulatorsImpl.dosboxXWorker(bundle), "dosboxXWorker", "dosbox-x");
}

function testServer(factory: CIFactory, name: string, backend: "dosbox" | "dosbox-x") {
    const ipxServerPort = defaultIpxServerPort;
    const globalIpxServerAddress = (window as any).ipxServerAddress;
    const ipxServerAddress = (typeof globalIpxServerAddress === "string" ?
        globalIpxServerAddress : defaultIpxServerAddress) + ":" + ipxServerPort + "/ipx/" + room;
    const ipxnetServerAddress = wsPrefix + ipxServerAddress + " " + ipxServerPort;

    suite(name + ".ipx");

    async function CI(bundle: DosBundle | Promise<DosBundle>) {
        bundle = await Promise.resolve(bundle);
        return await factory(await bundle.toUint8Array());
    }

    test(name + " should not freeze when connecting to wrong address (jsapi)", async () => {
        let notifiedDisconnected = false;
        const messages: string[] = [];
        const ci = await CI(await emulatorsImpl.bundle());
        assert.ok(ci);
        ci.events().onMessage((mType, message: string) => {
            messages.push(message);
        });
        ci.events().onNetworkDisconnected(() => {
            notifiedDisconnected = true;
        });

        try {
            await ci.networkConnect(NetworkType.NETWORK_DOSBOX_IPX, "127.0.0.1:1902/ipx/" + room);
            assert.ok(false, JSON.stringify(messages, null, 2));
        } catch (e) {
            assert.ok(notifiedDisconnected, "Disconnected is not notified");
        } finally {
            await ci.exit();
        }
    });

    test(name + " should connect to port " + ipxnetServerAddress + " (jsapi)", async () => {
        let notifiedConnected = false;
        let notifiedDisconnected = false;
        let connected = false;
        const messages: string[] = [];
        const ci = await CI(await emulatorsImpl.bundle());
        assert.ok(ci);
        ci.events().onMessage((mType, message: string) => {
            messages.push(message);
            connected = connected || message.startsWith("[LOG_NET]IPX: Connected to server.  IPX address is 127:0:0:1");
        });
        ci.events().onNetworkConnected(() => {
            notifiedConnected = true;
        });
        ci.events().onNetworkDisconnected(() => {
            notifiedDisconnected = true;
        });

        await ci.networkConnect(NetworkType.NETWORK_DOSBOX_IPX, ipxServerAddress);
        await ci.exit();

        assert.ok(connected, JSON.stringify(messages, null, 2));
        assert.ok(notifiedConnected, "Connected is not notified");
        assert.ok(notifiedDisconnected, "Disconnected is not notified");
    });

    test(name + " should connect to " + ipxnetServerAddress + " (ipxnet)", async () => {
        let notifiedConnected = false;
        let notifiedDisconnected = false;
        let connected = false;
        const messages: string[] = [];
        const ci = await CI((await emulatorsImpl.bundle())
            .autoexec("ipxnet connect " + ipxnetServerAddress));
        assert.ok(ci);
        ci.events().onMessage((mType, message: string) => {
            messages.push(message);
            connected = connected || message
                .startsWith("[LOG_NET]IPX: Connected to server.  IPX address is 127:0:0:1");
        });
        ci.events().onNetworkConnected(() => {
            notifiedConnected = true;
        });
        ci.events().onNetworkDisconnected(() => {
            notifiedDisconnected = true;
        });
        await sleep(backend === "dosbox-x" ? 3000 : 300);
        await ci.exit();

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
        const one = await CI((await emulatorsImpl.bundle())
            .autoexec("ipxnet connect " + ipxnetServerAddress));
        assert.ok(one);
        await sleep(backend === "dosbox-x" ? 3000 : 300);

        const two = await CI((await emulatorsImpl.bundle())
            .autoexec("ipxnet connect " + ipxnetServerAddress + "\nipxnet ping"));
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
        await one.exit();
        await two.exit();

        const usedPorts = Object.keys(portMap);
        assert(usedPorts.length === 1,
            "Should be 1 used port, but have " + JSON.stringify(usedPorts) + ":\n" +
            JSON.stringify(messages, null, 2));

        console.log("PING avg", ipxnetServerAddress, "is", Math.round(timeSumMs / timeSamples), "ms");
    });
}

async function sleep(timeMs: number) {
    return new Promise<void>((resolve) => {
        setTimeout(resolve, timeMs);
    });
}
