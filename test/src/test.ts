import { testLoader } from "./test-loader";
import { testLibZip } from "./test-libzip";
import { testConf } from "./test-conf";
import { testDosBundle } from "./test-bundle";
import { testDos } from "./test-dos";
// import { testNet } from "./test-net";

import emulatorsImpl from "../../src/impl/emulators-impl";

emulatorsImpl.pathPrefix = "/";

export function createTests() {
    testLoader();
    testLibZip();
    testConf();

    testDosBundle();
    testDos();
    // @caiiiycuk: TODO reenable
    // testNet();
}

(window as any).createTests = createTests;
