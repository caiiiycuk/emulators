# Tasks

## 1. Port the Fix and Add Regression Tests

- [x] 1.1 Inspect current user changes, the original patch SHA-256 from `design.md`, and `git apply --check ../js-dos/mcp/emulators-audio-lifecycle.patch`. Verification: the patch matches the recorded digest and applies; stop on any discrepancy and agree on the port without overwriting others' changes.
- [x] 1.2 Port the patch with the agreed error behavior into `audio-worklet.ts`, `direct.ts`, `worker.ts`, the new `test/src/test-audio-lifecycle.ts`, and registration in `test/src/node-runner.ts`. Verification: lifecycle scenarios pass, including repeated cleanup, audio creation failures, and startup failures in both transports; `ws-err` in both backends and `Worker.onerror`, including during audio creation, neither close audio nor interrupt startup. Preserve fallback, worker reinstallation without a prepared WASM module, and the public API.
- [x] 1.3 Add real AudioContext checks to the SDK browser suite for start, stop, and restart in DOSBox and DOSBox-X, direct and worker with native worklet enabled, including stop before the first gesture. Verification: `ws-err` preserves running audio in both backends, and `Worker.onerror` does not close audio; after stop, all instance contexts reach `closed`, gestures do not resume an old context, and only the new context is active on remount. Register the test in the standard browser run rather than leaving it as an external js-dos test.
- [x] 1.4 Run `yarn run eslint src --ext ts,tsx --max-warnings 0`, `yarn run eslint test --ext ts,tsx --max-warnings 0`, `yarn run tsc --noEmit`, and the required `yarn test:node:build`. Verification: commands pass, existing DSP and protocol checks are not weakened, and unrelated files and submodules remain unchanged.

## 2. Standard SDK Package

- [x] 2.1 Prepare the next available compatible patch version after `8.5.2` and an English `CHANGELOG.md` entry about the lifecycle fix. Verification: standard release metadata uses a consistent version, and the entry does not claim DSP changes, disabled worklet, or public API changes.
- [x] 2.2 Build the SDK with `yarn run gulp production`, run `yarn test:browser` with the new lifecycle check, and create an npm archive using standard `npm pack`. Verification: the browser suite passes, the archive contains rebuilt wrappers, required WASM, declarations, and the license; record its SHA-256. Leave the task open if the build or browser verification cannot run.
- [x] 2.3 Hand the verified package over for publication and publish only with separate user permission and npm access. Verification: the fixed version is actually available in the registry; record a blocker and leave this task open if access or permission is unavailable. Manual `node_modules` edits, local overrides, or copying JS into js-dos do not replace publication.

## 3. Verify Published Delivery

- [x] 3.1 Install the exact published version in an isolated consumer and repeat browser startup and lifecycle checks for DOSBox and DOSBox-X with native worklet. Verification: context closure and restart work with standard installed package files, without a patch hook or `JSDOS_TEST_EMULATORS_JS`; the neighboring js-dos remains unchanged.
- [x] 3.2 Save a report in the change with executed commands, results, version, source commit, and npm integrity, and compare implementation against every specification scenario. Verification: the data refers to the published SDK rather than the earlier prototype; version information is handed over to `add-jsdos-mcp`, without declaring its incomplete tasks complete or modifying its files.
