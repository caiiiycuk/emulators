const fs = require("fs");
require("../emulators");

const backend = process.argv[2] === "x" ? "dosboxXNode" : "dosboxNode";
const emulators = global.emulators;
emulators.pathPrefix = "./";

const bundle = fs.readFileSync("dhry2.jsdos");
let startedAt = null;
let runStartedAt = Date.now();
let prevNonSkippableSleepCount = 0;
let prevSleepCount = 0;
let prevCycles = 0;
const medianVax = [];
const medianCycles = [];
const medianNonSkippableSleepCount = [];
const medianSleepCount = [];
const sortFn = (a, b) => a - b;

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
                const dt = Date.now() - runStartedAt;
                const nonSkippableSleepCount = stats.nonSkippableSleepCount - prevNonSkippableSleepCount;
                const sleepCount = stats.sleepCount - prevSleepCount;
                const cycles = stats.cycles - prevCycles;
                prevNonSkippableSleepCount = stats.nonSkippableSleepCount;
                prevSleepCount = stats.sleepCount;
                prevCycles = stats.cycles;
                runStartedAt = Date.now();
                medianNonSkippableSleepCount.push(nonSkippableSleepCount * 1000 / dt);
                medianSleepCount.push(sleepCount * 1000 / dt);
                medianCycles.push(cycles / dt);
                console.log("dhry2: sleep p/sec: " + Math.round(sleepCount * 1000 / dt) +
                    " , non skippable p/sec: " + Math.round(nonSkippableSleepCount * 1000 / dt) +
                    " , avg cycles p/ms: " + Math.round(cycles / dt));
            });
            medianVax.push(vax);
            if (runs === "320000") {
                const executionTimeSec = (Date.now() - startedAt) / 1000;
                medianVax.sort(sortFn);
                medianNonSkippableSleepCount.sort(sortFn);
                medianSleepCount.sort(sortFn);
                medianCycles.sort(sortFn);

                console.log("Time:", Math.round(executionTimeSec * 10) / 10, "sec",
                    "RpS:", Math.round((runs - 1000) / executionTimeSec),
                    "Med VAX:", medianVax[Math.round(medianVax.length / 2)]);

                ci.asyncifyStats().then((stats) => {
                    console.log("Message sent:", stats.messageSent,
                        "(frame", stats.messageFrame + ")",
                        "(sound", stats.messageSound + ")");
                    console.log("Message recv:", stats.messageReceived);

                    console.log("Sleep count:", stats.sleepCount);
                    console.log("Sleep time:", Math.round(stats.sleepTime / 1000 * 10) / 10, "sec");
                    console.log("Avg sleep:", stats.sleepTime / stats.sleepCount, "ms");
                    console.log("Med non skippable sleep p/sec: ",
                        Math.round(medianNonSkippableSleepCount[Math.round(medianNonSkippableSleepCount.length / 2)]));
                    console.log("Med sleep p/sec:",
                        Math.round(medianSleepCount[Math.round(medianSleepCount.length / 2)]));
                    console.log("Med cycles p/ms:",
                        Math.round(medianCycles[Math.round(medianCycles.length / 2)]));

                    // properly exit
                    ci.exit();
                });
            }
        });
    })
    .catch(console.error);
