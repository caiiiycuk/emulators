import { assert } from "chai";
import { createServer } from "http";
import { AddressInfo } from "net";
import { Route } from "playwright";
import { brotliCompressSync } from "zlib";
import { isMissingCiv2Preload, routeCiv2Request } from "./civ2";

export function testCiv2Preload() {
    suite("Civilization II preload");
    test("optional misses skip invalid Brotli bodies without hiding other HTTP errors", async () => {
        const server = createServer((request, response) => {
            const status = Number(request.url!.split("/")[1]);
            response.writeHead(status, { "content-type": "application/json", "content-encoding": "br" });
            response.end(status === 403 || status === 404 ? "not found" : brotliCompressSync(Buffer.from("[]")));
        });
        await new Promise<void>((resolve) => server.listen(0, "127.0.0.1", resolve));
        try {
            for (const status of [403, 404, 200, 500]) {
                const url = "http://127.0.0.1:" + (server.address() as AddressInfo).port +
                    "/" + status + "/preload_ranges.metaj";
                let fulfilled: { status: number; body: string | Buffer; headers: Record<string, string> } | undefined;
                await routeCiv2Request({
                    request: () => ({ url: () => url, headers: () => ({}) }),
                    fetch: () => {
                        throw new Error("Optional metadata must not use route.fetch");
                    },
                    fulfill: async (options: typeof fulfilled) => {
                        fulfilled = options;
                    },
                } as unknown as Route);
                assert.strictEqual(fulfilled!.status, status);
                assert.strictEqual(fulfilled!.body.toString(), status === 403 || status === 404 ? "" : "[]");
                assert.strictEqual(fulfilled!.headers["access-control-allow-origin"], "*");
                assert.notProperty(fulfilled!.headers, "content-encoding");
                assert.strictEqual(isMissingCiv2Preload(url, status), status === 403 || status === 404);
            }
            assert.isFalse(isMissingCiv2Preload("https://br.cdn.dos.zone/disk.qcow2", 404));
        } finally {
            await new Promise<void>((resolve, reject) => server.close((error) => error ? reject(error) : resolve()));
        }
    });
}
