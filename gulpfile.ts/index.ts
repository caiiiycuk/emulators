import { series, parallel } from "gulp";
import { wasm } from "./wasm";
import { compileJs, emulators } from "./emulators";
import { test } from "./test";

import { emitTypes } from "./types";
import { updateDosbox } from "./update-dosbox";
import { dosboxAsyncify, dosboxXAsyncify } from "./asyncify";

function build(compress: boolean) {
    return series(
        wasm(compress),
        compileJs,
        parallel(emulators, test),
    );
}

exports.default = build(false);
exports.production = series(
    build(true),
    emitTypes,
);
exports.wasm = wasm(false);
exports.updateDosbox = updateDosbox;
exports.dosboxAsyncify = dosboxAsyncify;
exports.dosboxXAsyncify = dosboxXAsyncify;
