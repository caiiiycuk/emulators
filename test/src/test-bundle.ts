import { assert } from "chai";

import { createDosConfig, toDosboxConf } from "../../src/dos/bundle/dos-conf";
import DosBundle from "../../src/dos/bundle/dos-bundle";

import { makeLibZip, destroy } from "./libzip";
import LibZip from "../../src/libzip/libzip";

import emulators from "../../src/impl/emulators-impl";

async function toFs(bundle: DosBundle,
                    cb: (libzip: LibZip) => Promise<void>,
                    overwriteConfig = false) {
    const array = await bundle.toUint8Array(overwriteConfig);

    const unpacker = await makeLibZip();
    await unpacker.zipToFs(array);
    await cb(unpacker);
    destroy(unpacker);
}

export function testDosBundle() {
    suite("bundle");

    test("bundle should contain default dosbox.conf", async () => {
        await toFs(await emulators.dosBundle(), async (fs) => {
            const conf = await fs.readFile(".jsdos/dosbox.conf");
            assert.ok(conf);
            const expected = await toDosboxConf(createDosConfig());
            assert.equal(expected, conf);
        });
    });

    test("bundle should download and extract archive to root", async () => {
        const dosBundle = (await emulators.dosBundle())
            .extract("digger.zip", "/");

        await toFs(dosBundle, async (fs) => {
            const conf = await fs.readFile(".jsdos/dosbox.conf");
            const digger = await fs.readFile("/DIGGER.COM", "binary");
            assert.ok(conf);
            assert.ok(digger);
        });
    });

    test("bundle conf can be overwritten", async () => {
        const dosBundle = await (await emulators.dosBundle());
        const testPhrase = "overwritten by test";
        dosBundle.config.autoexec.options.script.value = testPhrase;
        dosBundle.extract("digger.jsdos", "/");

        await toFs(dosBundle, async (fs) => {
            const conf = (await fs.readFile(".jsdos/dosbox.conf", "utf8")) as string;
            assert.ok(conf.indexOf(testPhrase) === -1, "dosbox.conf should not contains test phrase");
        });

        await toFs(dosBundle, async (fs) => {
            const conf = (await fs.readFile(".jsdos/dosbox.conf", "utf8")) as string;
            assert.ok(conf.indexOf(testPhrase) > 0, "dosbox.conf should contains test phrase");
        }, true);
    });


    test("bundle should download and extract archive to path", async () => {
        const dosBundle = (await emulators.dosBundle())
            .extract("digger.zip", "test");

        await toFs(dosBundle, async (fs) => {
            const conf = await fs.readFile(".jsdos/dosbox.conf");
            const digger = await fs.readFile("/test/DIGGER.COM", "binary");
            assert.ok(conf);
            assert.ok(digger);
        });
    });

    test("bundle should extract multiple archive to paths", async () => {
        const dosBundle = (await emulators.dosBundle())
            .extract("digger.zip", "/test")
            .extract("arkanoid.zip", "/arkanoid");

        await toFs(dosBundle, async (fs) => {
            const conf = await fs.readFile(".jsdos/dosbox.conf");
            const digger = await fs.readFile("/test/DIGGER.COM", "binary");
            const arkanoid = await fs.readFile("/arkanoid/Arkanoid/ARKANOID.COM", "binary");
            assert.ok(conf);
            assert.ok(digger);
            assert.ok(arkanoid);
        });
    });

    test("can read bundle config", async () => {
        const libzip = await makeLibZip();
        libzip.writeFile("1", "1");
        let archive = await libzip.zipFromFs();
        libzip.destroy();
        assert.equal(await emulators.dosConfig(archive), null);

        const bundle = await emulators.dosBundle();
        archive = await bundle.toUint8Array();
        assert.equal(
            JSON.stringify(await emulators.dosConfig(archive), null, 2),
            JSON.stringify(bundle.config, null, 2));
    });
}
