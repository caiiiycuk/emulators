import { assert } from "chai";

import { audioWorkletCode } from "../../src/dos/dosbox/ts/audio-worklet";

interface WorkletPort {
    onmessage: ((event: { data: unknown }) => void) | null;
}

interface TestProcessor {
    port: WorkletPort;
    process(inputs: Float32Array[][], outputs: Float32Array[][]): boolean;
    bufferedFrames(): number;
}

type ProcessorConstructor = new () => TestProcessor;

export function testAudioWorklet() {
    suite("audio worklet");

    test("keeps pitch and bounded buffering across small speed changes", () => {
        const scenarios = [
            { sourceRate: 44100, speed: 0.98 },
            { sourceRate: 44100, speed: 1.00 },
            { sourceRate: 44100, speed: 1.01 },
            { sourceRate: 48000, speed: 1.00 },
        ];

        for (const scenario of scenarios) {
            const result = renderTone(scenario.sourceRate, scenario.speed);
            assert.approximately(result.frequency, 1000, 5,
                `unexpected pitch at source rate ${scenario.sourceRate}, speed ${scenario.speed}, ` +
                `tempo ${result.tempo}, buffered ${result.bufferedFrames}`);
            assert.isAtMost(result.maxBufferedFrames, 8192, "audio queue should stay bounded");
            assert.isBelow(result.maxStep, 0.5, "time stretching should not introduce sharp discontinuities");
            assert.isTrue(result.finite, "audio output should contain only finite samples");
        }
    });

    test("rebuffers after an underrun without replaying stale samples", () => {
        const processor = createProcessor(44100);
        send(processor, { type: "init", sourceRate: 44100 });
        send(processor, tone(0, 2560, 44100));

        const output = new Float32Array(128);
        for (let i = 0; i < 80; ++i) {
            processor.process([], [[output]]);
        }
        assert.approximately(output[output.length - 1], 0, 0.000001,
            "an underrun should fade all the way to silence");

        output.fill(1);
        send(processor, tone(2560, 1024, 44100));
        processor.process([], [[output]]);
        assert.deepEqual(Array.from(output), new Array(128).fill(0),
            "playback should wait for the full prebuffer after an underrun");

        send(processor, tone(3584, 1024, 44100));
        processor.process([], [[output]]);
        assert.isAbove(output.some((sample) => sample !== 0) ? 1 : 0, 0,
            "playback should resume after the prebuffer is restored");
    });
}

function createProcessor(outputRate: number): TestProcessor {
    let constructor: ProcessorConstructor | undefined;

    class FakeAudioWorkletProcessor {
        port: WorkletPort = { onmessage: null };
    }

    const registerProcessor = (_name: string, processor: ProcessorConstructor) => {
        constructor = processor;
    };
    const install = new Function("AudioWorkletProcessor", "registerProcessor", "sampleRate", audioWorkletCode);
    install(FakeAudioWorkletProcessor, registerProcessor, outputRate);
    assert.isDefined(constructor, "worklet should register its processor");
    return new constructor!();
}

function send(processor: TestProcessor, data: unknown) {
    assert.isNotNull(processor.port.onmessage, "processor should install a message handler");
    processor.port.onmessage!({ data });
}

function tone(start: number, length: number, sourceRate: number) {
    const samples = new Float32Array(length);
    for (let i = 0; i < length; ++i) {
        samples[i] = Math.sin((start + i) * 2 * Math.PI * 1000 / sourceRate) * 0.5;
    }
    return samples;
}

function renderTone(sourceRate: number, speed: number) {
    const outputRate = 44100;
    const processor = createProcessor(outputRate);
    send(processor, { type: "init", sourceRate });

    const quantum = 128;
    const durationSeconds = 6;
    const rendered = new Float32Array(outputRate * durationSeconds);
    let sourceBudget = 0;
    let sourcePosition = 0;
    let writePosition = 0;
    let maxBufferedFrames = 0;

    while (writePosition < rendered.length) {
        sourceBudget += quantum * sourceRate / outputRate * speed;
        while (sourceBudget >= 512) {
            send(processor, tone(sourcePosition, 512, sourceRate));
            sourcePosition += 512;
            sourceBudget -= 512;
        }

        const output = new Float32Array(quantum);
        processor.process([], [[output]]);
        rendered.set(output.subarray(0, Math.min(quantum, rendered.length - writePosition)), writePosition);
        writePosition += quantum;
        maxBufferedFrames = Math.max(maxBufferedFrames, processor.bufferedFrames());
    }

    const measured = rendered.subarray(outputRate, outputRate * 5);
    let crossings = 0;
    let maxStep = 0;
    let finite = true;
    for (let i = 1; i < measured.length; ++i) {
        if (measured[i - 1] <= 0 && measured[i] > 0) {
            crossings++;
        }
        maxStep = Math.max(maxStep, Math.abs(measured[i] - measured[i - 1]));
        finite = finite && Number.isFinite(measured[i]);
    }

    return {
        frequency: crossings / 4,
        maxBufferedFrames,
        maxStep,
        finite,
        tempo: (processor as any).tempo,
        bufferedFrames: processor.bufferedFrames(),
    };
}
