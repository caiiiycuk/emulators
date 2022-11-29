import * as process from "process";
import * as fs from "fs-extra";
import * as path from "path";
import { execute } from "./execute";

export default async function make(listsPath: string,
                                   buildPath: string,
                                   ...targets: string[]) {
    listsPath = path.resolve(listsPath);
    buildPath = path.resolve(buildPath);
    const cwd = path.resolve(process.cwd());

    if (!fs.existsSync(buildPath)) {
        fs.ensureDirSync(buildPath);
        process.chdir(buildPath);
        await emcmake(listsPath);
    }

    process.chdir(buildPath);
    await makeBuild(...targets);
    process.chdir(cwd);
}

async function makeBuild(...targets: string[]) {
    await execute("ninja", "-j4", ...targets);
}

async function emcmake(listsPath: string) {
    await execute("emcmake", "cmake", "-GNinja", "-DCMAKE_BUILD_TYPE=Release", listsPath);
}
