import { createReadStream, existsSync, statSync } from "fs";
import { createServer, IncomingMessage, Server, ServerResponse } from "http";
import { AddressInfo } from "net";
import { extname, join, resolve, sep } from "path";
import { chromium, Page } from "playwright";

interface BrowserLog {
    type: string;
    text: string;
    location: string;
}

interface BrowserMochaFailure {
    title: string;
    message: string;
    stack: string;
}

interface MochaResult {
    failureCount: number;
    failures: BrowserMochaFailure[];
}

interface BrowserTestMode {
    name: string;
    createTestsFunction: string;
    timeoutMs: number;
}

const repoRoot = resolve(__dirname, "../..");
const distRoot = join(repoRoot, "dist");
const testMode = getBrowserTestMode();
const maxBrowserLogs = Number(process.env.BROWSER_TEST_MAX_CONSOLE_LOGS ?? 200);

const requiredArtifacts = [
    "test/test.html",
    "test/test.js",
    "test/mocha.js",
    "test/chai.js",
    "test/mocha.css",
    "test/humblenet/jsapi.mjs",
    "test/humblenet/jsapi.wasm",
    "emulators.js",
    "wlibzip.js",
    "wlibzip.wasm",
    "wdosbox.js",
    "wdosbox.wasm",
    "wdosbox-x.js",
    "wdosbox-x.wasm",
    "wdosbox-x-jspi.js",
    "wdosbox-x-jspi.wasm",
];

const mimeTypes: Record<string, string> = {
    ".css": "text/css; charset=utf-8",
    ".html": "text/html; charset=utf-8",
    ".js": "application/javascript; charset=utf-8",
    ".jsdos": "application/octet-stream",
    ".mjs": "application/javascript; charset=utf-8",
    ".png": "image/png",
    ".wasm": "application/wasm",
    ".zip": "application/zip",
};

async function main() {
    assertRequiredArtifacts();

    const server = await startStaticServer();
    const baseUrl = getServerBaseUrl(server);
    let browser;

    try {
        browser = await chromium.launch({ headless: true });
        const page = await browser.newPage();
        const browserLogs: BrowserLog[] = [];
        let omittedBrowserLogs = 0;
        const pageErrors: string[] = [];
        const requestFailures: string[] = [];

        page.on("console", (message) => {
            if (browserLogs.length >= maxBrowserLogs) {
                browserLogs.shift();
                omittedBrowserLogs++;
            }

            browserLogs.push({
                type: message.type(),
                text: message.text(),
                location: formatConsoleLocation(message.location()),
            });
        });
        page.on("pageerror", (error) => {
            const formattedError = formatError(error).trim();

            if (formattedError.length > 0) {
                pageErrors.push(formattedError);
            }
        });
        page.on("requestfailed", (request) => {
            const failure = request.failure();
            requestFailures.push(request.url() + " " + (failure?.errorText ?? "failed"));
        });
        page.on("response", (response) => {
            if (response.status() >= 400) {
                requestFailures.push(response.status() + " " + response.url());
            }
        });

        await page.goto(baseUrl + "/test/test.html", { waitUntil: "load" });
        const mochaResult = await withTimeout(runBrowserTests(page, testMode), testMode.timeoutMs, testMode.name);
        const hasFailures = mochaResult.failureCount > 0 || pageErrors.length > 0;

        if (hasFailures) {
            printDiagnostics(testMode.name, mochaResult, pageErrors, requestFailures, browserLogs, omittedBrowserLogs);
            process.exitCode = 1;
            return;
        }

        console.log(testMode.name + " passed.");
        if (browserLogs.length > 0) {
            console.log("Browser console messages collected: " + (browserLogs.length + omittedBrowserLogs));
        }
    } catch (error) {
        console.error("Browser test runner failed:");
        console.error(formatError(error));
        process.exitCode = 1;
    } finally {
        await browser?.close();
        await closeServer(server);
    }
}

function getBrowserTestMode(): BrowserTestMode {
    if (process.argv.includes("--net")) {
        return {
            name: "Browser network tests",
            createTestsFunction: "createNetworkTests",
            timeoutMs: Number(process.env.BROWSER_NET_TEST_TIMEOUT_MS ??
                process.env.BROWSER_TEST_TIMEOUT_MS ??
                15 * 60 * 1000),
        };
    }

    return {
        name: "Browser tests",
        createTestsFunction: "createTests",
        timeoutMs: Number(process.env.BROWSER_TEST_TIMEOUT_MS ?? 15 * 60 * 1000),
    };
}

function assertRequiredArtifacts() {
    const missingArtifacts = requiredArtifacts.filter((artifact) => !existsSync(join(distRoot, artifact)));

    if (missingArtifacts.length > 0) {
        throw new Error("Missing browser test artifacts in dist: " + missingArtifacts.join(", ") +
            ". Run yarn run gulp production before yarn test:browser.");
    }
}

function startStaticServer(): Promise<Server> {
    const server = createServer((request, response) => serveStaticFile(request, response));

    return new Promise((resolveServer, reject) => {
        server.once("error", reject);
        server.listen(0, "127.0.0.1", () => {
            server.off("error", reject);
            resolveServer(server);
        });
    });
}

function serveStaticFile(request: IncomingMessage, response: ServerResponse) {
    try {
        const requestUrl = new URL(request.url ?? "/", "http://127.0.0.1");
        const requestedPath = decodeURIComponent(requestUrl.pathname);
        let filePath = resolve(distRoot, "." + requestedPath);

        if (filePath !== distRoot && !filePath.startsWith(distRoot + sep)) {
            sendStatus(response, 403, "Forbidden");
            return;
        }

        if (!existsSync(filePath)) {
            sendStatus(response, 404, "Not found");
            return;
        }

        if (statSync(filePath).isDirectory()) {
            filePath = join(filePath, "index.html");
        }

        if (!existsSync(filePath) || !statSync(filePath).isFile()) {
            sendStatus(response, 404, "Not found");
            return;
        }

        const fileStat = statSync(filePath);

        response.writeHead(200, {
            "Content-Type": mimeTypes[extname(filePath)] ?? "application/octet-stream",
            "Content-Length": fileStat.size,
        });
        createReadStream(filePath).pipe(response);
    } catch (error) {
        sendStatus(response, 500, formatError(error));
    }
}

function sendStatus(response: ServerResponse, statusCode: number, message: string) {
    response.writeHead(statusCode, {
        "Content-Type": "text/plain; charset=utf-8",
    });
    response.end(message);
}

function getServerBaseUrl(server: Server) {
    const address = server.address() as AddressInfo | null;

    if (address === null) {
        throw new Error("Static server did not start.");
    }

    return "http://127.0.0.1:" + address.port;
}

function closeServer(server: Server): Promise<void> {
    return new Promise((resolveClose, reject) => {
        server.close((error) => {
            if (error !== undefined) {
                reject(error);
                return;
            }

            resolveClose();
        });
    });
}

async function runBrowserTests(page: Page, mode: BrowserTestMode): Promise<MochaResult> {
    return await page.evaluate(async (createTestsFunction) => {
        const browserWindow = window as any;
        const testConfig = document.getElementById("test-config");

        if (typeof browserWindow[createTestsFunction] !== "function") {
            throw new Error("window." + createTestsFunction + " is not available.");
        }

        if (typeof browserWindow.mocha?.run !== "function") {
            throw new Error("mocha.run is not available.");
        }

        if (testConfig !== null) {
            testConfig.style.display = "none";
        }

        browserWindow[createTestsFunction]();

        return await new Promise((resolve) => {
            const failures: Array<{ title: string; message: string; stack: string }> = [];
            const runner = browserWindow.mocha.run((failureCount: number) => {
                resolve({ failureCount, failures });
            });

            runner.on("fail", (test: any, error: any) => failures.push({
                title: typeof test?.fullTitle === "function" ? test.fullTitle() : String(test?.title ?? "unknown test"),
                message: String(error?.message ?? error),
                stack: String(error?.stack ?? ""),
            }));
        });
    }, mode.createTestsFunction) as MochaResult;
}

function withTimeout<T>(promise: Promise<T>, ms: number, label: string): Promise<T> {
    return new Promise((resolvePromise, reject) => {
        const timer = setTimeout(() => reject(new Error(label + " timed out after " + ms + "ms.")), ms);

        promise.then(resolvePromise, reject).finally(() => clearTimeout(timer));
    });
}

function printDiagnostics(
    title: string,
    mochaResult: MochaResult,
    pageErrors: string[],
    requestFailures: string[],
    browserLogs: BrowserLog[],
    omittedBrowserLogs: number) {
    console.error(title + " failed.");

    if (mochaResult.failures.length > 0) {
        console.error("\nMocha failures:");
        for (const failure of mochaResult.failures) {
            console.error("- " + failure.title);
            console.error("  " + failure.message);
            if (failure.stack.length > 0) {
                console.error(failure.stack);
            }
        }
    } else if (mochaResult.failureCount > 0) {
        console.error("\nMocha reported " + mochaResult.failureCount + " failures without failure details.");
    }

    if (pageErrors.length > 0) {
        console.error("\nPage errors:");
        for (const error of pageErrors) {
            console.error(error);
        }
    }

    if (requestFailures.length > 0) {
        console.error("\nRequest failures:");
        for (const failure of requestFailures) {
            console.error(failure);
        }
    }

    if (browserLogs.length > 0) {
        console.error("\nBrowser console:");
        if (omittedBrowserLogs > 0) {
            console.error("Showing last " + browserLogs.length + " messages; omitted " + omittedBrowserLogs + ".");
        }

        for (const log of browserLogs) {
            console.error("[" + log.type + "] " + log.text + log.location);
        }
    }
}

function formatConsoleLocation(location: { url: string; lineNumber: number; columnNumber: number }) {
    if (location.url.length === 0) {
        return "";
    }

    return " (" + location.url + ":" + location.lineNumber + ":" + location.columnNumber + ")";
}

function formatError(error: unknown) {
    if (error instanceof Error) {
        return error.stack ?? error.message;
    }

    return String(error);
}

main().catch((error) => {
    console.error(formatError(error));
    process.exitCode = 1;
});
