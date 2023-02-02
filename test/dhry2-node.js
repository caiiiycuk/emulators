const fs = require("fs");
require("../emulators");

const backend = process.argv[2] === "x" ? "dosboxXNode" : "dosboxNode";
const emulators = global.emulators;
emulators.pathPrefix = "./";

const bundle = fs.readFileSync("dhry2.jsdos");
let startedAt = null;
let runStartedAt = Date.now();
let sleepCount = 0;
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
            ci.asyncifyStats().then((stats) => {
                const sleepPerSec = (stats.sleepCount - sleepCount) * 1000 / (Date.now() - runStartedAt);
                runStartedAt = Date.now();
                sleepCount = stats.sleepCount;
                console.log("dhry2: " + sleepPerSec + " sleepPerSec");
            });
            median.push(vax);
            if (runs === "320000") {
                const executionTimeSec = (Date.now() - startedAt) / 1000;
                median.sort();

                console.log("Time:", Math.round(executionTimeSec * 10) / 10, "sec",
                    "RpS:", Math.round((runs - 1000) / executionTimeSec),
                    "VAX:", median[median.length / 2]);

                ci.asyncifyStats().then((stats) => {
                    console.log("Message sent:", stats.messageSent,
                        "(frame", stats.messageFrame + ")",
                        "(sound", stats.messageSound + ")");
                    console.log("Message recv:", stats.messageReceived);

                    console.log("Sleep count:", stats.sleepCount);
                    console.log("Sleep time:", Math.round(stats.sleepTime / 1000 * 10) / 10, "sec");
                    console.log("Avg sleep:", stats.sleepTime / stats.sleepCount, "ms");
                    console.log("Avg sleep per sec:", stats.sleepCount * 1000 / (Date.now() - startedAt));

                    // properly exit
                    ci.exit();
                });
            }
        });
    })
    .catch(console.error);
