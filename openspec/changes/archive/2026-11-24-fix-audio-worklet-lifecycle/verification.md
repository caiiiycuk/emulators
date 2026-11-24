# Verification Report

## Status

On 2026-11-24, during archive confirmation, the user confirmed that all remaining tasks had been completed manually and authorized marking them complete. Tasks 2.3, 3.1, and 3.2 are therefore recorded as complete based on user confirmation, not new agent verification. Registry publication, published package checks, and release provenance were not independently rechecked during archiving; no new published version, source commit, or registry integrity was supplied.

The sections below preserve the earlier local verification record and its then-current publication blockers. At that time, local implementation and rechecks after clarification of `ws-err` behavior were complete: tasks 1.1, 1.2, 1.3, 1.4, 2.1, and 2.2. Tasks 2.3, 3.1, and 3.2 were still open because separate npm publication permission had not been given; npm permissions had not been checked and publication had not been performed by the agent. That record describes a local package, not a published SDK. Installing an archive in a separate consumer does not replace checking the exact registry version. The earlier assessment was that the change was not ready for archive; the later user confirmation above supersedes that task status.

## Sources and Version

- Repository: `/home/caiiiycuk/js-dos/emulators`, branch `8.xx`.
- Source commit: `23c3849d971311262a1370303844146d42b76b9a`, checked through Git. At the time of this local record, the fix was in uncommitted changes without a separate fixed-version commit.
- Original patch: `../js-dos/mcp/emulators-audio-lifecycle.patch`.
- Patch SHA-256: `d55d13fc32c0b0e87eda5e36efddbbc8eb61f23c1b73a83c2bd17f0943d65807`, checked with `sha256sum`. `git apply --check` passed before porting.
- Version: `emulators@8.5.3`, the next available patch version after `8.5.2` at the time of the `npm view` check. Recorded in `package.json`, with an English entry added to `CHANGELOG.md`.
- The standard production build generated `Build.version = "8.5.3 (888a2a9f0785f8bdf2d5ce4676fccc2b)"` in `src/build.ts`.

Source file SHA-256 values after porting:

```text
1249c677bea6e1ffb424acd0f7c70d4288b769f3fe07c09e473d221ed6d38f8b  src/dos/dosbox/ts/audio-worklet.ts
08930c40026568f433f6bf1826a812be77d7babd6dca76c859f72ce38c544267  src/dos/dosbox/ts/direct.ts
54577715bf7ca283b02d9a6ed77869d21cdbd4e3fd93161614ec443cb2e39c02  src/dos/dosbox/ts/worker.ts
```

## Preserving Audio After Error Messages

Rechecking found a regression in the ported patch: `worker.onerror` called `transportLayer.exit()`, forcibly terminating the worker before its `ws-exit` response. The real `CommandInterface.exit()` remained pending after such an error. A Worker error does not necessarily stop it.

Per the user's clarification, both audio cleanup and the startup failure flag were removed from `onerror`. The event is only forwarded as the existing `ws-err` message. Transport creation continues even if the event occurs during `await createAudioPort()`. Audio is released on `exit()` and initialization rejection, not on an error message.

Before the final fix, the regression test failed: `Worker.onerror` produced a close count of 1 instead of 0. After the fix, 16 lifecycle checks pass. The real `CommandInterfaceOverTransportLayer` receives `ws-err` with normal and `panic` tags after backend readiness in direct and worker modes. Messages reach `events().onMessage()` with type `error`, but the context, port, node, and gesture handlers remain intact. Subsequent exit resolves the Promise, reports termination once, and releases audio. `Worker.onerror` during audio creation and operation is checked separately, including with worklet disabled.

No similar regression was found in direct mode. Errors from `instantiate` and synchronous `callMain` clean up resources before CI creation and preserve the original error. Normal cleanup runs after `ws-exit`, so it does not interrupt the response expected by CI. No further `direct.ts` changes were needed.

## Executed Commands

| Command | Result |
| --- | --- |
| `yarn run eslint src --ext ts,tsx --max-warnings 0` | Passed |
| `yarn run eslint test --ext ts,tsx --max-warnings 0` | Passed |
| `yarn run tsc --noEmit` | Passed |
| `yarn run tsc --noEmit --target esnext --module commonjs --moduleResolution node --strict --skipLibCheck --esModuleInterop test/src/browser-audio-lifecycle.ts` | Separate browser test type check passed |
| `yarn test:node:build` | Passed, 84 tests including 16 lifecycle tests |
| `yarn run gulp production` | Passed, generated wrappers, WASM, and declarations |
| `yarn test:browser` | Passed, including lifecycle checks for all four backends and the existing suite |
| `npm pack --json --pack-destination /tmp/opencode` | Created archive through the standard process |
| `git diff --check` | Passed |
| `openspec validate fix-audio-worklet-lifecycle --strict --json` | Passed without errors |

The final `test:node:build`, production build, and browser suite ran sequentially, without changing `dist` during browser verification. Logs are at `/tmp/opencode/audio-lifecycle-{node-build,production,browser}-verified.log`.

The browser test is in `test/src/browser-audio-lifecycle.ts` and is called from the standard `test/src/browser-runner.ts`. It uses native AudioContext and AudioWorkletNode rather than mocks. The first instance is created through CDP with `userGesture: false` because Playwright `page.evaluate()` activates the page. Checks confirm no prior activation and the `suspended` state.

For each of `dosboxDirect`, `dosboxWorker`, `dosboxXDirect`, and `dosboxXWorker`, checks cover stop before the first gesture and two subsequent starts. After each stop, all contexts reach `closed`, each node is disconnected once, and pointerdown and keydown do not resume old contexts. All previous contexts remain closed during the next start.

Both transports receive `ws-err` through the existing backend channel; worker mode also sends a synthetic `ErrorEvent` to the real Worker after startup. Checks cover message delivery to CI, preservation of the `running` context, no node disconnection or premature `terminate`, and subsequent completion of real `ci.exit()` and context closure. The installed Worker script is not replaced. This scenario checks recoverable error handling, not a Chromium process crash.

## Specification Scenario Coverage

| Scenario | Evidence |
| --- | --- |
| Stop a running instance | Node checks for port.close, disconnect, and handler removal; native browser check of `closed` for all four backends |
| Stop before the first user gesture | Native browser test confirms initial `suspended`, then `closed`, and no resume after gestures |
| Worklet loading or node creation fails | Node tests confirm context closure and `undefined` return; both transports preserve fallback without an audio port |
| Backend initialization fails | Node tests cover direct instantiate, callMain, and both worker install attempts; the original callMain error is preserved |
| A running backend reports ws-err | Direct and worker Node tests check normal and panic message delivery to CI without cleanup; native browser test checks running audio preservation |
| Worker error during audio creation | Node test confirms successful installation and preservation of the late context until subsequent exit |
| Error in a running worker | Node test confirms resource preservation and ws-err at the transport boundary; real CI and native Worker preserve audio and termination through ws-exit |
| Repeated termination | Node tests confirm cleanup occurs once on repeated exit and after worker error; repeated CI exit does not repeat the exit event |
| Create, stop, and recreate cycle | Native browser restart checks pass for all four backends |
| Use the standard package | Local npm archive verified; at the time of this record, the published version requirement was blocked by task 2.3 |
| Verify backward compatibility | Node and browser suites passed; DSP, public API, and protocol were unchanged |

Byte-for-byte equality with the source commit was also checked for `audioWorkletCode`, `src/emulators.ts`, `src/protocol/protocol.ts`, `test/src/test-audio-worklet.ts`, and `test/src/test-protocol.ts`. Worker reinstallation without a prepared WASM module preserves the same audio port.

## Local npm Archive

Archive: `/tmp/opencode/emulators-8.5.3.tgz`.

Data from `npm pack --json` and `sha256sum`:

```text
size: 9783532 bytes
unpackedSize: 29055584 bytes
SHA-256: 48de410b8d44f6f61f5681de8e7268215ead4d52b66dc861731aba2cf6983e71
npm shasum: f85de070e3b2dec66bb8da0f61912da4925fd919
npm integrity: sha512-paHJw3GFwkPj21dP2v/wcdBCHKRsiyJwWBZmDsGVEVEjHHa5KawVLL1lVNGkyXY03Dq8YVZRZdGqHrKPnUJtmg==
```

The archive contains 38 files, including `dist/emulators.js`, wrappers and WASM for DOSBox, DOSBox-X, debug, JSPI, and libzip, `dist/types/emulators.d.ts`, and `LICENSE`. `.npmignore` excludes local tools, OpenSpec, talks, sockdrive, and browser check results that appeared in the trial archive. User files themselves were not changed.

The archive was installed into `/tmp/opencode/audio-lifecycle-consumer-errors-preserved` using:

```bash
npm install --prefix /tmp/opencode/audio-lifecycle-consumer-errors-preserved --ignore-scripts --no-audit --no-fund /tmp/opencode/emulators-8.5.3.tgz
node /tmp/opencode/verify-audio-lifecycle-package.cjs
```

Verification confirmed the installed archive integrity and byte-for-byte equality of all its files with the production build. Native browser lifecycle scenarios passed for both cores, direct and worker, using standard installed package files. Log: `/tmp/opencode/audio-lifecycle-installed-package-verified.log`. No hooks, `node_modules` edits, overrides, or `JSDOS_TEST_EMULATORS_JS` were used.

## Publication Blocker and Consumer Handoff at Local Verification Time

Task 2.3 required separate user permission for npm publication and npm access. After permission, the fix commit needed confirmation, the package needed rebuilding if release metadata changed, and availability of the exact registry version needed checking.

Task 3.1 required installing the published version itself and repeating native browser checks. After verification, the report needed the registry integrity and published fix commit before completing task 3.2.

A candidate version `emulators@8.5.3` was prepared for `add-jsdos-mcp`. Switching to it required publication first. Files in the neighboring js-dos and `add-jsdos-mcp` were not changed, nor were their tasks declared complete. The `native/dosbox-x` submodule retained its pre-existing untracked `.idea/`; no other submodule changes were made.

`yarn test:browser:net` and checks in other browsers were not run. Native lifecycle verification was limited to the project's standard Chromium runner.

The existing message format produced by `Worker.onerror` was unchanged. It lacks `sessionId`, so `CommandInterfaceOverTransportLayer.onServerMessage()` drops the message. This separate defect predates the lifecycle patch; preservation is checked at the transport boundary. Normal backend `ws-err` messages with `sessionId` are checked through real `ci.events().onMessage()` with type `error`.
