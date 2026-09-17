export async function createAudioPort(): Promise<MessagePort | undefined> {
    let url: string | undefined;

    try {
        const blob = new Blob([audioWorkletCode], { type: "application/javascript" });
        url = URL.createObjectURL(blob);
        const context = new AudioContext({ latencyHint: "interactive" });
        await context.audioWorklet.addModule(url);
        const node = new AudioWorkletNode(context, "jsdos-audio", {
            numberOfInputs: 0,
            numberOfOutputs: 1,
            outputChannelCount: [1],
        });
        node.connect(context.destination);

        const resumeWebAudio = () => {
            if (context.state === "suspended") {
                context.resume().catch(console.error);
            }
        };
        document.addEventListener("pointerdown", resumeWebAudio, { once: true, capture: true });
        document.addEventListener("keydown", resumeWebAudio, { once: true, capture: true });

        return node.port;
    } catch (e) {
        console.error("error creating audio port", e);
        return undefined;
    } finally {
        if (url !== undefined) {
            URL.revokeObjectURL(url);
        }
    }
}

// Exported so the exact AudioWorklet program can be exercised by the Node test suite.
export const audioWorkletCode = `
class SampleRing {
    constructor(capacity) {
        this.samples = new Float32Array(capacity);
        this.readPosition = 0;
        this.writePosition = 0;
        this.samplesCount = 0;
    }

    clear() {
        this.readPosition = 0;
        this.writePosition = 0;
        this.samplesCount = 0;
    }

    length() {
        return this.samplesCount;
    }

    push(sample) {
        if (this.samplesCount === this.samples.length) {
            this.discard(1);
        }
        this.samples[this.writePosition] = sample;
        this.writePosition = (this.writePosition + 1) % this.samples.length;
        this.samplesCount++;
    }

    pushArray(samples) {
        for (let i = 0; i < samples.length; ++i) {
            this.push(samples[i]);
        }
    }

    peek(offset) {
        return this.samples[(this.readPosition + offset) % this.samples.length];
    }

    shift() {
        const sample = this.peek(0);
        this.discard(1);
        return sample;
    }

    discard(count) {
        count = Math.min(count, this.samplesCount);
        this.readPosition = (this.readPosition + count) % this.samples.length;
        this.samplesCount -= count;
    }
}

class Processor extends AudioWorkletProcessor {
    constructor() {
        super();
        this.outputRate = sampleRate;
        this.sourceRate = sampleRate;
        this.targetFrames = Math.max(1024, Math.round(this.outputRate * 2048 / 44100));
        this.maxFrames = this.targetFrames * 4;
        this.frameSize = 512;
        this.hopSize = this.frameSize / 2;
        this.searchSize = 64;
        this.sourceQueue = new SampleRing(this.maxFrames * 4);
        this.queue = new SampleRing(this.maxFrames);
        this.outputQueue = new SampleRing(this.frameSize * 4);
        this.tail = new Float32Array(this.hopSize);
        this.sourcePosition = 0;
        this.tempo = 1;
        this.analysisDebt = 0;
        this.started = false;
        this.haveTail = false;
        this.lastOutput = 0;

        this.port.onmessage = (event) => {
            const message = event.data;
            if (message instanceof Float32Array) {
                this.sourceQueue.pushArray(message);
                this.resampleInput();
            } else if (message && message.type === "init") {
                this.initialize(message.sourceRate);
            }
        };
    }

    initialize(sourceRate) {
        this.sourceRate = sourceRate > 0 ? sourceRate : this.outputRate;
        this.sourceQueue.clear();
        this.queue.clear();
        this.outputQueue.clear();
        this.sourcePosition = 0;
        this.tempo = 1;
        this.analysisDebt = 0;
        this.started = false;
        this.haveTail = false;
        this.lastOutput = 0;
    }

    resampleInput() {
        const step = this.sourceRate / this.outputRate;
        while (this.sourcePosition + 1 < this.sourceQueue.length()) {
            const index = Math.floor(this.sourcePosition);
            const fraction = this.sourcePosition - index;
            const first = this.sourceQueue.peek(index);
            const second = this.sourceQueue.peek(index + 1);
            this.queue.push(first + (second - first) * fraction);
            this.sourcePosition += step;
        }

        const consumed = Math.floor(this.sourcePosition);
        if (consumed > 0) {
            this.sourceQueue.discard(consumed);
            this.sourcePosition -= consumed;
        }
    }

    prepareFirstFrame() {
        if (this.queue.length() < this.frameSize) {
            return false;
        }
        for (let i = 0; i < this.hopSize; ++i) {
            this.outputQueue.push(this.queue.peek(i));
            this.tail[i] = this.queue.peek(this.hopSize + i);
        }
        this.haveTail = true;
        return true;
    }

    updateTempo() {
        const error = (this.queue.length() - this.targetFrames) / this.targetFrames;
        const desired = Math.max(0.95, Math.min(1.05, 1 + error * 0.1));
        const change = Math.max(-0.002, Math.min(0.002, desired - this.tempo));
        this.tempo += change;
    }

    findBestFrame(expected) {
        const minimum = Math.max(1, expected - this.searchSize);
        const maximum = Math.min(
            expected + this.searchSize,
            this.queue.length() - this.frameSize,
        );
        if (maximum < minimum) {
            return -1;
        }

        let bestPosition = Math.min(expected, maximum);
        let bestScore = Number.POSITIVE_INFINITY;
        for (let position = minimum; position <= maximum; ++position) {
            let difference = 0;
            for (let i = 0; i < this.hopSize; i += 4) {
                const delta = this.tail[i] - this.queue.peek(position + i);
                difference += delta * delta;
            }
            const distancePenalty = Math.abs(position - expected) * 0.002;
            const score = difference + distancePenalty;
            if (score < bestScore) {
                bestScore = score;
                bestPosition = position;
            }
        }
        return bestPosition;
    }

    generateFrame() {
        if (!this.haveTail) {
            return this.prepareFirstFrame();
        }

        this.updateTempo();
        const idealAdvance = this.hopSize * this.tempo;
        const expected = Math.max(1, Math.round(idealAdvance + this.analysisDebt));
        const position = this.findBestFrame(expected);
        if (position < 0) {
            return false;
        }
        this.analysisDebt += idealAdvance - position;
        this.analysisDebt = Math.max(-this.searchSize, Math.min(this.searchSize, this.analysisDebt));

        this.queue.discard(position);
        for (let i = 0; i < this.hopSize; ++i) {
            const mix = i / this.hopSize;
            const sample = this.tail[i] * (1 - mix) + this.queue.peek(i) * mix;
            this.outputQueue.push(sample);
            this.tail[i] = this.queue.peek(this.hopSize + i);
        }
        return true;
    }

    stopForUnderflow() {
        this.started = false;
        this.haveTail = false;
        this.outputQueue.clear();
        this.tempo = 1;
        this.analysisDebt = 0;
    }

    writeTo(output) {
        if (!this.started && this.queue.length() >= this.targetFrames) {
            this.started = true;
        }

        while (this.started && this.outputQueue.length() < output.length) {
            if (!this.generateFrame()) {
                break;
            }
        }

        let written = 0;
        while (written < output.length && this.outputQueue.length() > 0) {
            this.lastOutput = this.outputQueue.shift();
            output[written++] = this.lastOutput;
        }

        if (written < output.length) {
            const missing = output.length - written;
            for (let i = 0; i < missing; ++i) {
                output[written + i] = this.lastOutput * (1 - (i + 1) / missing);
            }
            this.lastOutput = 0;
            if (this.started) {
                this.stopForUnderflow();
            }
        }
    }

    bufferedFrames() {
        return this.queue.length() + this.outputQueue.length();
    }

    process(inputs, outputs) {
        const channels = outputs[0];
        this.writeTo(channels[0]);
        for (let channel = 1; channel < channels.length; ++channel) {
            channels[channel].set(channels[0]);
        }
        return true;
    }
}

registerProcessor("jsdos-audio", Processor);
`;
