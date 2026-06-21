import { existsSync } from "fs";
import { join, resolve } from "path";
import { assert } from "chai";

import emulatorsImpl from "../../src/impl/emulators-impl";
import { loadWasmModule } from "../../src/impl/modules";

import { testLibZip } from "./test-libzip";
import { testDosBundle } from "./test-bundle";
import { testDos } from "./test-dos";

const repoRoot = resolve(__dirname, "../..");
const testRoot = join(repoRoot, "test");
const wasmRoot = join(repoRoot, "build/wasm");

const requiredArtifacts = [
    "wlibzip.js",
    "wlibzip.wasm",
    "wdosbox.js",
    "wdosbox.wasm",
    "wdosbox-x.js",
    "wdosbox-x.wasm",
];
const missingArtifacts = requiredArtifacts.filter((artifact) => !existsSync(join(wasmRoot, artifact)));

if (missingArtifacts.length > 0) {
    throw new Error("Missing Node test WASM artifacts in build/wasm: " +
        missingArtifacts.join(", ") + ". Run the existing wasm/build task before yarn test:node.");
}

process.chdir(testRoot);
emulatorsImpl.pathPrefix = wasmRoot;

if (typeof (globalThis as any).ImageData === "undefined") {
    (globalThis as any).ImageData = class ImageData {
        public data: Uint8ClampedArray;
        public width: number;
        public height: number;

        constructor(data: Uint8ClampedArray, width: number, height: number) {
            this.data = data;
            this.width = width;
            this.height = height;
        }
    };
}

suite("WASM loader.node");
test("loader can load wdosbox.js from build/wasm", async () => {
    const module = await loadWasmModule(join(wasmRoot, "wdosbox.js"), "WDOSBOX_NODE_SMOKE",
        () => {/**/});
    assert.ok(module);
    assert.isFunction(module.instantiate);
});

testLibZip();
testDosBundle();
testDos([
    {
        factory: (bundle, options) => emulatorsImpl.dosboxNode(bundle, options),
        name: "dosboxNode",
        assets: "dosbox",
    },
    {
        factory: (bundle, options) => emulatorsImpl.dosboxXNode(bundle, options),
        name: "dosboxXNode",
        assets: "dosbox-x",
    },
]);
