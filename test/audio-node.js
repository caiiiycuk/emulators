"use strict";
exports.__esModule = true;
exports.audioNode = void 0;
var SamplesQueue = /** @class */ (function () {
    function SamplesQueue() {
        this.samplesQueue = [];
    }
    SamplesQueue.prototype.push = function (samples) {
        this.samplesQueue.push(samples);
    };
    SamplesQueue.prototype.length = function () {
        var total = 0;
        for (var _i = 0, _a = this.samplesQueue; _i < _a.length; _i++) {
            var next = _a[_i];
            total += next.length;
        }
        return total;
    };
    SamplesQueue.prototype.writeTo = function (dst, bufferSize) {
        var writeIt = 0;
        while (this.samplesQueue.length > 0) {
            var src = this.samplesQueue[0];
            var toRead = Math.min(bufferSize - writeIt, src.length);
            if (toRead === src.length) {
                dst.set(src, writeIt);
                this.samplesQueue.shift();
            }
            else {
                dst.set(src.slice(0, toRead), writeIt);
                this.samplesQueue[0] = src.slice(toRead);
            }
            writeIt += toRead;
            if (writeIt === bufferSize) {
                break;
            }
        }
        if (writeIt < bufferSize) {
            dst.fill(0, writeIt);
        }
    };
    return SamplesQueue;
}());
function audioNode(ci) {
    var sampleRate = ci.soundFrequency();
    var channels = 1;
    if (sampleRate === 0) {
        console.warn("Can't create audio node with sampleRate === 0, ingnoring");
        return;
    }
    var audioContext = null;
    if (typeof AudioContext !== "undefined") {
        audioContext = new AudioContext({
            sampleRate: sampleRate,
            latencyHint: "interactive"
        });
    }
    else if (typeof window.webkitAudioContext !== "undefined") {
        // eslint-disable-next-line new-cap
        audioContext = new window.webkitAudioContext({
            sampleRate: sampleRate,
            latencyHint: "interactive"
        });
    }
    if (audioContext == null) {
        return;
    }
    var samplesQueue = new SamplesQueue();
    var bufferSize = 2048;
    var preBufferSize = 2048;
    ci.events().onSoundPush(function (samples) {
        if (samplesQueue.length() < bufferSize * 2 + preBufferSize) {
            samplesQueue.push(samples);
        }
    });
    var audioNode = audioContext.createScriptProcessor(bufferSize, 0, channels);
    var started = false;
    var active = 0;
    var onQueueProcess = function (event) {
        var numFrames = event.outputBuffer.length;
        var numChannels = event.outputBuffer.numberOfChannels;
        var samplesCount = samplesQueue.length();
        if (!started) {
            started = samplesCount >= preBufferSize;
        }
        if (!started) {
            return;
        }
        for (var channel = 0; channel < numChannels; channel++) {
            var channelData = event.outputBuffer.getChannelData(channel);
            samplesQueue.writeTo(channelData, numFrames);
        }
    };
    audioNode.onaudioprocess = onQueueProcess;
    audioNode.connect(audioContext.destination);
    var resumeWebAudio = function () {
        if (audioContext !== null && audioContext.state === "suspended") {
            audioContext.resume();
        }
    };
    document.addEventListener("click", resumeWebAudio, { once: true });
    document.addEventListener("touchstart", resumeWebAudio, { once: true });
    document.addEventListener("keydown", resumeWebAudio, { once: true });
    ci.events().onExit(function () {
        if (audioContext !== null) {
            audioNode.disconnect();
            audioContext.close();
        }
        document.removeEventListener("click", resumeWebAudio);
        document.removeEventListener("touchstart", resumeWebAudio);
        document.removeEventListener("keydown", resumeWebAudio);
    });
}
exports.audioNode = audioNode;
