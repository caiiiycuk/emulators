import * as fs from "fs";

import { src, dest, series, parallel } from "gulp";
import del from "del";

import sourcemaps from "gulp-sourcemaps";
import terser from "gulp-terser";
import size from "gulp-size";
import browserify from "browserify";
import buffer from "vinyl-buffer";
import source from "vinyl-source-stream";

// eslint-disable-next-line
const tsify = require("tsify");
// eslint-disable-next-line
const footer = require("gulp-footer");

function clean() {
    return del(["dist/emulators*",
        "build/wworker-footer*"], { force: true });
}

function js() {
    return browserify({
        debug: true,
        entries: ["src/emulators.ts"],
        cache: {},
        packageCache: {},
    })
        .plugin(tsify, {
            "target": "esnext",
        })
        .transform("babelify", {
            presets: [["@babel/preset-env", {
                "useBuiltIns": "usage",
                "corejs": 3,
            }]],
            extensions: [".ts"],
        })
        .bundle()
        .pipe(source("emulators.js"))
        .pipe(buffer())
        .pipe(sourcemaps.init({ loadMaps: true }))
        .pipe(terser())
        .pipe(sourcemaps.write("./"))
        .pipe(size({ showFiles: true, showTotal: false }))
        .pipe(dest("dist"));
}

function dosboxJs() {
    return src("dist/wdosbox.js")
        .pipe(footer(fs.readFileSync("src/dos/dosbox/ts/worker-server.js")))
        .pipe(dest("dist"));
}

export const emulators = series(clean, parallel(js, dosboxJs));
