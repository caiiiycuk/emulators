# Audio resource ownership in transports

## Context

The motivation is described in `proposal.md`. A design is needed because the fix affects the shared audio helper, both transports, and asynchronous worker initialization.

Before the fix, `createAudioPort()` returns only a `MessagePort`, while references to the `AudioContext`, `AudioWorkletNode`, and gesture handlers remain inside the helper. The transport's `exit()` cannot release these resources. `dosDirect()` also lacks shared cleanup for failed initialization. A `ws-err` message or `Worker.onerror` event alone does not mean the backend has terminated.

The prepared patch is at `/home/caiiiycuk/js-dos/js-dos/mcp/emulators-audio-lifecycle.patch` (relative to this repository: `../js-dos/mcp/emulators-audio-lifecycle.patch`). Its SHA-256 is:

`d55d13fc32c0b0e87eda5e36efddbbc8eb61f23c1b73a83c2bd17f0943d65807`

The patch was prepared against official commit `6bbf0925d221a7304ea8e0dda1e89b8dc01569b2`. When the change was created, `git apply --check` passed against SDK commit `23c3849d971311262a1370303844146d42b76b9a`. This confirms applicability only, not test results for the current checkout.

## Goals and Constraints

Change only internal resource ownership and add checks. The public API, WASM protocol, worklet algorithm, and default enablement remain unchanged. Native worklet must not be replaced with ScriptProcessor to make the lifecycle test pass.

This change is scoped to emulators. User changes, `native/dosbox-x`, the neighboring js-dos, and its `node_modules` remain untouched. Switching the dependency and removing the consumer workaround belong to `add-jsdos-mcp`, not this change.

## Decisions

### Internal audio descriptor instead of a port alone

`createAudioPort()` returns `{ port, dispose } | undefined`. The helper owns the context, node, and gesture handlers. `dispose` removes handlers, closes the available port, disconnects the node, and starts `context.close()` with error handling. References are cleared so repeated calls are safe. Context closure remains an asynchronous Web Audio operation; tests wait for `closed`, not merely a method call.

The same disposer handles partially completed audio creation. The existing `undefined` fallback and blob URL release in `finally` are preserved.

A new public cleanup method was rejected: a transport owner already exists, and consumers should not have to repair the SDK lifecycle manually.

### Each transport releases its own descriptor

The direct transport stores the descriptor, passes only the port to `module.audioPort`, and calls the disposer from its `exit()`. Errors from `instantiate` and `callMain` go through the same `exit()`, preserving the original error and removal of the existing `message` listener.

The worker transport stores the descriptor, passes only the port to the worker, and cleans up audio on `exit()`. The `worker.onerror` handler only sends the existing `ws-err` message, without audio cleanup, forced worker termination, or a startup failure flag. A handler error does not necessarily stop the worker, and `CommandInterface.exit()` must receive `ws-exit` before transport termination. Neither backend cleans up on `ws-err` messages. The existing `wc-install` retry without a precompiled WASM module is preserved; `catch` performs cleanup if that attempt also throws.

Cleanup only on normal `ci.exit()` was rejected because it does not close resources when failure occurs before CI is obtained.

### The SDK has its own regression checks

The patch adds `test/src/test-audio-lifecycle.ts` and registration in `test/src/node-runner.ts`. Checks cover normal exit, repeated exit, and startup failure for direct/worker, plus worklet loading and node creation failures. Per the user's clarification, worker error tests verify audio preservation during creation and operation, not cleanup. In both backends, `ws-err` preserves audio and reaches the existing message handler. Mocked browser globals are restored in `finally`.

The SDK browser suite also checks real `AudioContext` instances: audio remains running after `ws-err`, consecutive starts and stops close old contexts, and remount leaves no second active context. DOSBox and DOSBox-X are checked with native worklet enabled in both transports; worker checks also cover `Worker.onerror`. Mock tests do not replace this check, and counting CI instances is insufficient.

The previous 76 Node tests and browser checks of both backends passed in an isolated prototype inside js-dos. These are porting inputs, not grounds to mark the current change's tasks complete.

### Standard SDK delivery

The patch is applied to SDK sources once. It does not become a build/postinstall hook, runtime substitution, or permanent dependency on a neighboring checkout.

After verification, choose an available compatible patch version after `8.5.2`, update normal release metadata, and build the SDK with `yarn run gulp production`. Inspect the npm archive to confirm it contains rebuilt wrappers, WASM, declarations, and the license rather than stale `dist` files. Check the standard package in an isolated consumer without replacing dependency files or using `JSDOS_TEST_EMULATORS_JS`.

The apply command alone does not authorize npm publication or a public release: separate user approval and credentials are required. Without permission or access, the published delivery task remains open. Record the version, source commit, and npm integrity after publication so js-dos can pin that version in its own change.

## Risks and Checks

- A worker error during `addModule()` does not prove backend termination → separately test preservation of the late context and its cleanup on subsequent `exit()`.
- A passing Node mock does not prove browser audio thread release → check real contexts and native worklet in the browser.
- The SDK branch differs from the patch base → check checksum, applicability, and diff; do not overwrite unrelated changes.
- Stale `dist` files can hide a missing package fix → run a production build and check the installed npm archive, not a JS override alone.
- Publication may be unavailable → explicitly record the blocker and do not declare the consumer unblocked.

## Release Order and Rollback

First port the change and tests, then run lint, TypeScript, the required `yarn test:node:build`, and browser lifecycle checks. Then build and verify the npm archive, and publish only with separate permission.

Do not publish if verification fails. After publication, rolling the consumer back to the previous version is a separate action that restores the known leak; do not hide regressions by disabling native worklet. This change does not declare `add-jsdos-mcp` complete: it retains its own checks and external blockers.
