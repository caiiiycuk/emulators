import * as process from "process";
import * as fs from "fs-extra";
import * as path from "path";
import { cpus } from "os";
import { execute } from "./execute";

export default async function make(listsPath: string,
                                   buildPath: string,
                                   target: string,
                                   debug = false) {
    listsPath = path.resolve(listsPath);
    buildPath = path.resolve(buildPath);
    const cwd = path.resolve(process.cwd());

    if (!fs.existsSync(buildPath)) {
        fs.ensureDirSync(buildPath);
        process.chdir(buildPath);
        await emcmake(listsPath, debug);
    }

    process.chdir(buildPath);
    await makeBuild(target);
    process.chdir(cwd);
}

async function makeBuild(...targets: string[]) {
    await execute("ninja", "-j" + cpus().length, ...targets);
}

async function emcmake(listsPath: string, debug: boolean) {
    await execute("emcmake", "cmake", "-GNinja", "-DCMAKE_BUILD_TYPE=Release",
        "-DDOSBOX_DEBUG=" + (debug ? "ON" : "OFF"), listsPath);
}
