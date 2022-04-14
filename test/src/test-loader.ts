/* eslint @typescript-eslint/no-unused-vars: 0 */

import { assert } from "chai";

import { loadWasmModule, host } from "../../src/impl/modules";

export function testLoader() {
    suite("WASM loader");

    test("loader should rejects when wasm not supported", async () => {
        host.wasmSupported = false;
        try {
            await loadWasmModule("wrongurl.js", "", () => {/**/});
            host.wasmSupported = true;
            assert.fail();
        } catch (e) {
            host.wasmSupported = true;
            assert.equal("Starting from js-dos 6.22.60 js environment is not supported", e.message);
        }
    });

    test("loader should rejects when js file not exists", async () => {
        try {
            await loadWasmModule("wrongurl.js", "", () => {/**/});
            assert.fail();
        } catch (e) {
            assert.equal("Unable to download 'wrongurl.wasm', code: 404", e.message);
        }
    });

    test("loader should show progress loading", async () => {
        const moduleUrl = "/wdosbox.js";

        await loadWasmModule(moduleUrl, "",
            (stage: string, total: number, loaded: number) => {
                assert.equal(true, loaded <= total, "onprgoress: " + loaded + "<=" + total);
            });
    });
}
