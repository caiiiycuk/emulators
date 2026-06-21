import { assert } from "chai";

import { WasmModulesImpl } from "../../src/impl/modules";
import emulatorsImpl from "../../src/impl/emulators-impl";
import LibZip from "../../src/libzip/libzip";

export async function makeLibZip(module?: any) {
    module = module || {};
    const pathPrefix = emulatorsImpl.pathPrefix === "" ? "/" : emulatorsImpl.pathPrefix;
    const wasm = await new WasmModulesImpl(pathPrefix, "", "", "", "").libzip();
    await wasm.instantiate(module);
    return new LibZip(module, "/home/web_user");
}

export function destroy(libzip: LibZip) {
    const exitStatus = libzip.destroy();
    assert.equal(exitStatus.name, "ExitStatus");
    assert.equal(exitStatus.status, 0);
}
