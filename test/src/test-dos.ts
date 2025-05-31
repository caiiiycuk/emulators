/* eslint-disable new-cap */

import { assert } from "chai";
import { renderComparsionOf, waitImage } from "./compare";

import DosBundle from "../../src/dos/bundle/dos-bundle";
import { BackendOptions, CommandInterface, InitFs, PersistedSockdrives } from "../../src/emulators";
import emulatorsImpl from "../../src/impl/emulators-impl";

import { httpRequest } from "../../src/http";

import { Keys } from "../../src/keys";
import { makeLibZip } from "./libzip";
import { Build } from "../../src/build";
import emulators from "../../src/impl/emulators-impl";

type CIFactory = (bundle: InitFs, options?: BackendOptions) => Promise<CommandInterface>;

export function testDos() {
    testServer((bundle, options) => emulatorsImpl.dosboxDirect(bundle, options), "dosboxDirect", "dosbox");
    testServer((bundle, options) => emulatorsImpl.dosboxWorker(bundle, options), "dosboxWorker", "dosbox");
    testServer((bundle, options) => emulatorsImpl.dosboxXDirect(bundle, options), "dosboxXDirect", "dosbox-x");
    testServer((bundle, options) => emulatorsImpl.dosboxXWorker(bundle, options), "dosboxXWorker", "dosbox-x");
}

function testServer(factory: CIFactory, name: string, assets: string) {
    suite(name + ".common");
    beforeEach(() => {
        (Mocha as any).process.removeListener("uncaughtException");
    });

    async function CI(bundle: DosBundle | Promise<DosBundle>, options?: BackendOptions) {
        bundle = await Promise.resolve(bundle);
        return await factory(await bundle.toUint8Array(), options);
    }

    test(name + " can read dosbox.conf from string", async () => {
        const expected = "[sdl]\ntest_prop=1";
        const ci = await factory(expected);
        const actual = new TextDecoder().decode(await ci.fsReadFile(".jsdos/dosbox.conf"));
        assert.equal(actual, expected);
        await ci.exit();
    });

    test(name + " can read dosbox.conf from DosConfig", async () => {
        const dosboxConf = "[sdl]\ntest_prop=1";
        const jsdosConf = { version: "test" };
        const ci = await (await factory({
            dosboxConf,
            jsdosConf,
        }));
        const decoder = new TextDecoder();
        assert.equal(decoder.decode(await ci.fsReadFile(".jsdos/dosbox.conf")), dosboxConf);
        assert.equal(decoder.decode(await ci.fsReadFile(".jsdos/jsdos.json")), JSON.stringify(jsdosConf, null, 2));
        await ci.exit();
    });

    test(name + " can track extract progress", async () => {
        const actual: string[] = [];
        const ci = await CI(emulatorsImpl.bundle(), {
            onExtractProgress: (index, file, extracted, count) => {
                actual.push(index + " " + file + " " + extracted + " " + count);
            },
        });
        assert.ok(ci);
        assert.deepEqual(actual, [
            "0 .jsdos/ 1 4",
            "0 .jsdos/dosbox.conf 2 4",
            "0 .jsdos/readme.txt 3 4",
            "0 .jsdos/jsdos.json 4 4",
        ]);
        await ci.exit();
    });

    test(name + " can take screenshot of dosbox", async () => {
        const ci = await CI(emulatorsImpl.bundle());
        assert.ok(ci);
        await waitImage(assets + "/init.png", ci, { threshold: 0 });
    });

    test(name + " should not start without jsdos conf", async () => {
        try {
            const buffer = await httpRequest("digger.zip", {
                responseType: "arraybuffer",
            });
            await factory(new Uint8Array(buffer as ArrayBuffer));
            assert.fail();
        } catch (e) {
            assert.equal("[\"Broken bundle, .jsdos/dosbox.conf not found\"]", e.message);
        }
    });

    test(name + " should provide config back to js", async () => {
        const bundle = await emulatorsImpl.bundle();
        const ci = await CI(bundle);
        assert.ok(ci);
        const config = await ci.config();
        assert.equal(config.dosboxConf, bundle.dosboxConf);
        assert.equal(JSON.stringify(config.jsdosConf), JSON.stringify(bundle.jsdosConf));
        await ci.exit();
    });

    test(name + " can config js-dos through initFs", async () => {
        const dosboxConf = "[autoexec]\necho \"ok\"\n";
        const jsdosConf = "{\"initFs\":true}";
        const ci = await factory(
            [
                { path: ".jsdos/dosbox.conf", contents: new TextEncoder().encode(dosboxConf) },
                { path: ".jsdos/jsdos.json", contents: new TextEncoder().encode(jsdosConf) },
            ],
        );
        assert.ok(ci);
        const config = await ci.config();
        assert.equal(config.dosboxConf, dosboxConf);
        assert.equal(JSON.stringify(config.jsdosConf), jsdosConf);
        await ci.exit();
    });

    suite(name + ".persistency");
    async function testChangesFile(changes: Uint8Array, fileName: string, contents: string) {
        if (changes === null) {
            assert.fail("changes is null");
        }
        const libzip = await makeLibZip();
        libzip.zipToFs(changes);
        assert.ok(libzip.exists(fileName), fileName + " not exists");
        const content = await libzip.readFile(fileName);
        libzip.destroy();
        assert.equal(content, contents);
    }

    test(name + " should not return empty updates", async () => {
        const bundle = await emulators.bundle();
        const ci = await CI(bundle);
        assert.ok(ci);
        const changes = await ci.persist();
        assert.ok(changes === null, "changes not empty!");
        await ci.exit();
    });

    test(name + " should store fs updates between sessions [empty db/existent db]", async () => {
        let cachedBundle: Uint8Array = new Uint8Array();
        {
            const buffer = await httpRequest("helloworld.jsdos", {
                responseType: "arraybuffer",
            });

            const ci = await factory(new Uint8Array(buffer as ArrayBuffer));
            assert.ok(ci);
            assert.ok(cachedBundle, "cachedBundle is undefined");
            await waitImage(assets + "/persistent-mount.png", ci, {
                success: async () => {
                    cachedBundle = await ci.persist() as Uint8Array;
                    await testChangesFile(cachedBundle, "HW.TXT", "HELLO, WROLD!\r\n");
                },
            });
        }

        {
            const buffer = await httpRequest("helloworld.jsdos", {
                responseType: "arraybuffer",
            });

            const ci = await factory([new Uint8Array(buffer as ArrayBuffer), cachedBundle]);
            assert.ok(ci);
            cachedBundle = new Uint8Array();
            await waitImage(assets + "/persistent-mount-second.png", ci, {
                success: async () => {
                    await testChangesFile(await ci.persist() as Uint8Array,
                        "HW.TXT", "HELLO, WROLD!\r\nHELLO, WROLD!\r\n");
                },
            });
        }
    });

    test(name + " should track new files [existent db]", async () => {
        let changes: Uint8Array | PersistedSockdrives | null = null;

        {
            const bundle = await (await emulatorsImpl.bundle()).toUint8Array();
            const ci = await factory([bundle]);
            assert.ok(ci);
            await ci.config();
            await ci.fsWriteFile("File1.txt", new TextEncoder().encode("FILE1\n"));
            changes = await ci.persist();
            await testChangesFile(changes as Uint8Array, "File1.txt", "FILE1\n");
            await ci.exit();
        }

        {
            const bundle = await (await emulatorsImpl.bundle()).toUint8Array();
            const ci = await factory([bundle, changes as Uint8Array]);
            assert.ok(ci);
            await ci.config();
            try {
                assert.equal(new TextDecoder().decode(await ci.fsReadFile("File1.txt")), "FILE1\n");
            } catch (e) {
                console.log(e);
                assert.fail("File1.txt not found");
            }
            await ci.fsWriteFile("File2.txt", new TextEncoder().encode("FILE2\n"));
            changes = await ci.persist();
            await testChangesFile(changes as Uint8Array, "File1.txt", "FILE1\n");
            await testChangesFile(changes as Uint8Array, "File2.txt", "FILE2\n");
            await ci.exit();
        }
    });

    suite(name + ".fs");
    test(name + " can browse fs tree", async () => {
        const ci = await CI((await emulatorsImpl.bundle())
            .extract("digger.zip"));
        assert.ok(ci);
        const fsTree = await ci.fsTree();
        const expected = JSON.stringify({
            "name": ".",
            "nodes": [
                {
                    "name": ".jsdos",
                    "nodes": [
                        {
                            "name": "dosbox.conf",
                            "nodes": null,
                            "size": 7825,
                        },
                        {
                            "name": "readme.txt",
                            "nodes": null,
                            "size": 306,
                        },
                        {
                            "name": "jsdos.json",
                            "nodes": null,
                            "size": JSON.stringify({
                                version: Build.version,
                            }, null, 2).length,
                        },
                    ],
                    "size": null,
                },
                {
                    "name": "DIGGER.COM",
                    "nodes": null,
                    "size": 57856,
                },
            ],
            "size": null,
        }, null, 2);
        const actual = JSON.stringify(fsTree, null, 2);
        assert.equal(actual, expected);

        await ci.exit();
    });

    test(name + " can read the file from fs", async () => {
        const ci = await CI((await emulatorsImpl.bundle())
            .extract("digger.zip"));
        assert.ok(ci);
        const file = await ci.fsReadFile(".jsdos/jsdos.json");
        assert.ok(file);
        assert.equal(new TextDecoder().decode(file), JSON.stringify({ version: Build.version }, null, 2));
        await ci.exit();
    });

    test(name + " can write file and then read it from fs", async () => {
        const ci = await CI((await emulatorsImpl.bundle())
            .extract("digger.zip"));
        assert.ok(ci);
        const contents = "The js-dos v8 is absoultely badass";
        await ci.fsWriteFile("dynamic/jsdos.v8", new TextEncoder().encode(contents));
        const fsContents = await ci.fsReadFile("dynamic/jsdos.v8");
        assert.equal(new TextDecoder().decode(fsContents), contents);
        await ci.exit();
    });

    test(name + " can delete files and folders from fs", async () => {
        const ci = await CI((await emulatorsImpl.bundle())
            .extract("digger.zip"));
        assert.ok(ci);
        assert.ok((await ci.fsDeleteFile("not-existing-file.txt")) === false, "should return false if file not exists");
        assert.ok(await ci.fsDeleteFile("DIGGER.COM"), "should return true if file exists and deleted");
        assert.ok(await ci.fsDeleteFile(".jsdos/dosbox.conf"), "able to delete file in subfolder");
        assert.ok(await ci.fsDeleteFile(".jsdos"), "able to delete folder with files");

        const fsTree = await ci.fsTree();
        const expected = JSON.stringify({
            "name": ".",
            "nodes": [],
            "size": null,
        }, null, 2);
        const actual = JSON.stringify(fsTree, null, 2);
        assert.equal(actual, expected);

        await ci.exit();
    });

    suite(name + ".game");

    test(name + " can run digger.jsdos", async () => {
        const ci = await CI((await emulatorsImpl.bundle())
            .extract("digger.zip")
            .autoexec("DIGGER.COM"));
        assert.ok(ci);
        await waitImage("digger.png", ci, { timeout: 5000, resize: assets === "dosbox-x" });
    });

    test(name + " can play sound", async () => {
        const ci = await CI((await emulatorsImpl.bundle())
            .extract("digger.zip")
            .autoexec("DIGGER.COM"));
        assert.ok(ci);
        assert.equal(ci.soundFrequency(), 44100, "sound frequency should be 22050");

        const samples = await new Promise<Float32Array>((resolve) => {
            ci.events().onSoundPush((samples: Float32Array) => {
                resolve(samples);
            });
        });

        assert.ok(samples.byteLength > 0, "samples is empty");
        await ci.exit();
    });

    test(name + " exit event", async () => {
        const ci = await CI((await emulatorsImpl.bundle())
            .extract("digger.zip")
            .autoexec("DIGGER.COM"));
        assert.ok(ci);
        const exitPromise = new Promise<void>((resolve) => {
            ci.events().onExit(() => {
                resolve();
            });
        });
        await ci.exit();
        await exitPromise;
        assert.ok(true);
    });

    test(name + " can pause/resume emulation", async () => {
        const ci = await CI((await emulatorsImpl.bundle())
            .extract("digger.zip")
            .autoexec("DIGGER.COM"));
        assert.ok(ci);

        await new Promise((resolve) => setTimeout(resolve, 1000));
        ci.pause();

        await new Promise((resolve) => setTimeout(resolve, 300));
        const first = await ci.screenshot();

        await new Promise((resolve) => setTimeout(resolve, 300));
        const second = await ci.screenshot();
        ci.resume();

        await new Promise((resolve) => setTimeout(resolve, 300));
        const third = await ci.screenshot();

        await ci.exit();

        function compare(a: ImageData, b: ImageData) {
            for (let i = 0; i < a.data.length; ++i) {
                if (a.data[i] !== b.data[i]) {
                    return false;
                }
            }

            return true;
        }

        if (!compare(first, second)) {
            renderComparsionOf(first, second);
            assert.fail("screenshot during pause is changed");
        }

        if (compare(first, third)) {
            console.log(first === third, first, third);
            renderComparsionOf(first, third);
            assert.fail("screenshot during emulation is not changed");
        }
    });

    test(name + " can simulate key events", async () => {
        const ci = await CI((await emulatorsImpl.bundle())
            .extract("digger.zip")
            .autoexec("DIGGER.COM"));
        assert.ok(ci);

        await new Promise((resolve, reject) => {
            const keyPress = () => {
                ci.simulateKeyPress(Keys.KBD_left);
            };

            const screenshot = () => {
                waitImage("digger-end.png", ci, {
                    threshold: 2, resize: assets === "dosbox-x", timeout: 5000,
                })
                    .then(resolve)
                    .catch(reject);
            };

            setTimeout(keyPress, 2000);
            setTimeout(screenshot, 3000);
        });
    });

    test(name + " can simulate key combination", async () => {
        const ci = await CI((await emulatorsImpl.bundle())
            .extract("digger.zip")
            .autoexec("DIGGER.COM"));
        assert.ok(ci);

        const addKeyEventCalled = [];
        let initialTime = 0;
        (ci as any).addKey = (keyCode, pressed, timeMs) => {
            if (initialTime === 0) {
                initialTime = timeMs;
            }
            addKeyEventCalled.push([keyCode, pressed, timeMs]);
        };

        await new Promise<void>((resolve, reject) => {
            const keyPress = () => {
                ci.simulateKeyPress(Keys.KBD_left, Keys.KBD_leftctrl, Keys.KBD_leftshift);
            };

            const assertKeyPressSequence = () => {
                assert.deepEqual(addKeyEventCalled, [
                    [Keys.KBD_left, true, initialTime],
                    [Keys.KBD_leftctrl, true, initialTime],
                    [Keys.KBD_leftshift, true, initialTime],
                    [Keys.KBD_left, false, initialTime + 16],
                    [Keys.KBD_leftctrl, false, initialTime + 16],
                    [Keys.KBD_leftshift, false, initialTime + 16],
                ]);
                resolve();
            };

            setTimeout(keyPress, 2000);
            setTimeout(assertKeyPressSequence, 2100);
        });

        await ci.exit();
    });

    test(name + " can simulate mouse events", async () => {
        const buffer = await httpRequest("mousetst.jsdos", {
            responseType: "arraybuffer",
        });

        const ci = await factory(new Uint8Array(buffer as ArrayBuffer));
        assert.ok(ci);

        await new Promise((resolve, reject) => {
            const interactFn = () => {
                ci.sendMouseMotion(380 / 640, 250 / 400);
                ci.sendMouseButton(0, true);

                waitImage(assets + "/mousetst.png", ci, { threshold: 2 })
                    .then(resolve)
                    .catch(reject);
            };

            setTimeout(interactFn, assets === "dosbox" ? 1000 : 3000);
        });
    });
}
