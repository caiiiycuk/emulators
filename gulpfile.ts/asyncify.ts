import { log, error, warn } from "./log";
import * as fs from "fs";

const asyncifyFile = "targets/dosbox-asyncify.txt";
const stackFile = "/tmp/stack.txt";

export async function asyncifyAdd() {
    if (!fs.existsSync(stackFile)) {
        error("Stack file " + stackFile + " does not exists, exitinig...");
        return;
    }

    const content = fs.readFileSync(asyncifyFile, "utf8");
    const functions = JSON.parse(content);
    const functionsSet: { [name: string]: number } = {};
    for (const next of functions) {
        const simpleName = extractSimpleName(next);
        if (simpleName !== null) {
            functionsSet[simpleName] = 1;
        }
    }

    const entries = fs.readFileSync(stackFile, "utf8").split("\n");
    const newEntries: string[] = [];
    for (const next of entries) {
        const fnSig = extractFnSignature(next);
        const simpleName = extractSimpleName(fnSig);
        if (simpleName !== null && functionsSet[simpleName] === undefined) {
            newEntries.push(simpleName);
            functionsSet[simpleName] = 1;
        }
    }

    if (newEntries.length === 0) {
        log("No new entires, exiting...");
    }

    log("== asyncify list");
    const sorted = Object.keys(functionsSet).sort();
    let outcome = "[";
    for (const next of sorted) {
        outcome += "\"" + next + "\",";
        log("\t'" + next + "'");
    }

    for (const next of sorted) {
        outcome += "\"" + next + "(*)*\",";
    }

    log("== New entries:");
    newEntries.sort();
    for (const next of newEntries) {
        log("\t'" + next + "'");
    }

    outcome = outcome.substring(0, outcome.length - 1) + "]";
    fs.writeFileSync(asyncifyFile, outcome, "utf8");

    log("Well done...");
}

function extractFnSignature(next: string): string {
    next = next.trim();
    if (next.length === 0) {
        return "";
    }

    let fnSig = "";

    // case 1
    const index = next.indexOf("@");
    if (index >= 0) {
        fnSig = next.substr(0, index).trim();
    }

    // case 2
    if (fnSig.length === 0) {
        const matched = /at\s+(.*)\s+\([<|\d]/.exec(next);
        if (matched) {
            fnSig = matched[1];
        }
    }

    // case 3
    if (fnSig.length === 0) {
        const matched = /\s*(.*)\s+http:\/\//.exec(next);
        if (matched) {
            fnSig = matched[1];
        }
    }

    fnSig = fnSig.trim();

    if (fnSig.length == 0) {
        warn("Unparsable entry '" + next + "'");
        return "";
    }

    if (fnSig[0] === "$") {
        fnSig = fnSig.substring(1);
    }

    if (fnSig.indexOf("\"") !== -1) {
        warn("Entry contains \" '" + next + "', skipping...");
        return "";
    }

    return fnSig;
}

function extractSimpleName(fnName: string | null | undefined): string | null {
    if (fnName === null || fnName === undefined || fnName.length === 0) {
        return null;
    }

    const end = fnName.indexOf("(");
    if (end === -1) {
        return fnName;
    }

    return fnName.substring(0, end);
}
