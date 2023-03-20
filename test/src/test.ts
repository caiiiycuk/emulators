import { testLoader } from "./test-loader";
import { testLibZip } from "./test-libzip";
import { testDosBundle } from "./test-bundle";
import { testDos } from "./test-dos";
import { testNet } from "./test-net";

import emulatorsImpl from "../../src/impl/emulators-impl";

emulatorsImpl.pathPrefix = "/";

export function createTests() {
    testLoader();
    testLibZip();

    testDosBundle();
    testDos();
}

export function createNetworkTests() {
    testNet();
}

(window as any).createTests = createTests;
(window as any).createNetworkTests = createNetworkTests;
