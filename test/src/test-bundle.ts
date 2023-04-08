import { assert } from "chai";

import DosBundle, { defaultConfig } from "../../src/dos/bundle/dos-bundle";

import { makeLibZip, destroy } from "./libzip";
import LibZip from "../../src/libzip/libzip";

import emulators from "../../src/impl/emulators-impl";
import { Build } from "../../src/build";

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
        await toFs(await emulators.bundle(), async (fs) => {
            const dosboxConf = await fs.readFile(".jsdos/dosbox.conf");
            const jsdosConf = JSON.parse(await fs.readFile(".jsdos/jsdos.json") as string);
            assert.ok(dosboxConf);
            assert.ok(jsdosConf);
            assert.equal(defaultConfig, dosboxConf);
            assert.equal(JSON.stringify({ version: Build.version }),
                JSON.stringify(jsdosConf));
        });
    });

    test("bundle should download and extract archive to root", async () => {
        const dosBundle = (await emulators.bundle())
            .extract("digger.zip", "/");

        await toFs(dosBundle, async (fs) => {
            const conf = await fs.readFile(".jsdos/dosbox.conf");
            const digger = await fs.readFile("/DIGGER.COM", "binary");
            assert.ok(conf);
            assert.ok(digger);
        });
    });

    test("bundle conf can be overwritten", async () => {
        const dosBundle = await (await emulators.bundle());
        const testPhrase = "overwritten by test";
        dosBundle.dosboxConf += "\n#" + testPhrase;
        dosBundle.extract("helloworld.jsdos", "/");

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
        const dosBundle = (await emulators.bundle())
            .extract("digger.zip", "test");

        await toFs(dosBundle, async (fs) => {
            const conf = await fs.readFile(".jsdos/dosbox.conf");
            const digger = await fs.readFile("/test/DIGGER.COM", "binary");
            assert.ok(conf);
            assert.ok(digger);
        });
    });

    test("bundle should extract multiple archive to paths", async () => {
        const dosBundle = (await emulators.bundle())
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
        assert.equal(await emulators.bundleConfig(archive), null);

        const bundle = await emulators.bundle();
        archive = await bundle.toUint8Array();
        assert.equal(
            (await emulators.bundleConfig(archive)).dosboxConf,
            bundle.dosboxConf);
        assert.equal(
            JSON.stringify((await emulators.bundleConfig(archive)).jsdosConf),
            JSON.stringify(bundle.jsdosConf));
    });

    test("can update bundle config", async () => {
        const bundle = await emulators.bundle();
        const newBundle = await emulators.bundleUpdateConfig(await bundle.toUint8Array(), {
            dosboxConf: "[sdl]",
            jsdosConf: { version: "0" },
        });
        const config = await emulators.bundleConfig(newBundle);
        assert.equal("[sdl]", config.dosboxConf);
        assert.equal(JSON.stringify({ version: "0" }), JSON.stringify(config.jsdosConf));
    });
}
