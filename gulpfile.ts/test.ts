import { src, dest, series, parallel } from "gulp";
import del from "del";

import size from "gulp-size";
import browserify from "browserify";
import buffer from "vinyl-buffer";
import source from "vinyl-source-stream";

// eslint-disable-next-line
const tsify = require("tsify");

function clean() {
    return del(["dist/test*", "dist/test/temp.jsdos"], { force: true });
}

function copyTestAssets() {
    return src(["test/*.html", "test/*.png", "test/*.zip", "test/*.jsdos",
        "test/dhry2-node.js",
        "test/mocha.css", "test/mocha.js", "test/chai.js",
        "test/adapter-latest.js",
        "test/stats.min.js", "test/audio-node.js", "test/webgl.js"])
        .pipe(dest("dist/test"));
}

function copyTestAssetsDosbox() {
    return src(["test/dosbox/*"])
        .pipe(dest("dist/test/dosbox"));
}

function copyTestAssetsDosboxX() {
    return src(["test/dosbox-x/*"])
        .pipe(dest("dist/test/dosbox-x"));
}

function testJs() {
    return browserify({
        debug: true,
        entries: ["test/src/test.ts"],
        cache: {},
        packageCache: {},
    })
        .plugin(tsify, {
            "target": "esnext",
            "esModuleInterop": false,
            "allowSyntheticDefaultImports": true,
            "strict": false,
            "forceConsistentCasingInFileNames": false,
            "resolveJsonModule": true,
            "isolatedModules": false,
        })
        .transform("babelify", {
            presets: [["@babel/preset-env", {
                "useBuiltIns": "usage",
                "corejs": 3,
            }]],
            extensions: [".ts"],
        })
        .bundle()
        .pipe(source("test.js"))
        .pipe(buffer())
        .pipe(size({ showFiles: true, showTotal: false }))
        .pipe(dest("dist/test"));
}

export const test = series(clean, parallel(copyTestAssets, copyTestAssetsDosbox,
    copyTestAssetsDosboxX, testJs));
