const fs = require("fs");
require("../emulators");

const emulators = global.emulators;
emulators.pathPrefix = "./";

const bundle = fs.readFileSync("dhry2.jsdos");

emulators
    .dosboxNode(bundle)
    .then((ci) => {
        ci.events().onStdout((message) => {
            if (!message.startsWith("dhry2:")) {
                return;
            }

            const [_, runs, delta, vax] = message.split(" ");
            console.log("dhry2: " + runs + " runs, browser time " + delta + " ms, " +
                        "VAX rating " + vax);
            if (Number.parseInt(delta, 10) >= 5000) {
                //properly exit
                ci.exit();
            }
        });
    })
    .catch(console.error);
