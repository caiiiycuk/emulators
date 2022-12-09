const fs = require("fs");
require("../emulators");

const backend = process.argv[2] === "x" ? "dosboxXNode" : "dosboxNode";
const emulators = global.emulators;
emulators.pathPrefix = "./";

const bundle = fs.readFileSync("dhry2.jsdos");
let startedAt = null;
const median = [];

console.log("Dhrystone 2 Test for " + backend);
emulators[backend](bundle)
    .then((ci) => {
        ci.events().onStdout((message) => {
            if (!message.startsWith("dhry2:")) {
                return;
            }

            if (startedAt === null) {
                startedAt = Date.now();
            }

            const [_, runs, delta, vax] = message.split(" ");
            console.log("dhry2: " + runs + " runs, browser time " + delta + " ms, " +
                "VAX rating " + vax);
            median.push(vax);
            if (runs === "320000") {
                const executionTimeSec = (Date.now() - startedAt) / 1000;
                median.sort();

                console.log("Time:", Math.round(executionTimeSec * 10) / 10, "sec",
                    "RpS:", Math.round((runs - 1000) / executionTimeSec),
                    "VAX:", median[median.length / 2]);

                console.log("Message sent:", ci.transport.module.messageSent,
                    "(frame", ci.transport.module.messageFrame + ")",
                    "(sound", ci.transport.module.messageSound + ")");
                console.log("Message recv:", ci.transport.module.messageReceived);

                console.log("Sleep count:", ci.transport.module.sleep_count);
                console.log("Sleep time:", Math.round(ci.transport.module.sleep_time / 1000 * 10) / 10, "sec");
                console.log("Avg sleep:", ci.transport.module.sleep_time / ci.transport.module.sleep_count, "ms");

                // properly exit
                ci.exit();
            }
        });
    })
    .catch(console.error);
