# Native AudioWorklet resource cleanup

## Why

In `emulators 8.5.2`, stopping the emulator leaves open `AudioContext` instances, connected `AudioWorkletNode` instances, and audio resume handlers. This prevents correct unmount/remount behavior in Studio and generated projects in the neighboring js-dos change `add-jsdos-mcp`. The fix must live in the SDK and ship as a standard package rather than a local consumer patch.

## What Changes

- Port the prepared fix from `../js-dos/mcp/emulators-audio-lifecycle.patch` into SDK sources after checking applicability to the current branch.
- Give internal transport owners of audio resources an idempotent disposer. Release resources on `exit` and initialization rejection. A `ws-err` message in either backend or a `Worker.onerror` event alone must not close audio or interrupt startup.
- Add direct/worker regression tests and browser checks of real contexts across restarts.
- Prepare a standard package for the next patch release. Publication requires separate permission and npm access; until then, switching js-dos to the published version remains blocked.

## Capabilities

### New Capabilities

- `audio-worklet-lifecycle`: own and release native AudioWorklet resources on stop and failed startup without changing the public API or audio processing algorithm.

### Modified Capabilities

None: there are no existing capabilities in `openspec/specs/` at proposal time.

## Impact

Affected areas are `src/dos/dosbox/ts/{audio-worklet,direct,worker}.ts`, Node test registration, and a new lifecycle test. Release requires a standard SDK production build and consistent package version metadata.

The public `CommandInterface`, `BackendOptions`, and WASM message format remain unchanged. Native worklet stays enabled; C/C++, DSP, third-party submodules, and user changes are out of scope.

After release, the neighboring js-dos must separately update its pinned dependency and provenance information as part of `add-jsdos-mcp`. This change does not modify js-dos files, its `node_modules`, or release automation and does not automatically publish to npm.
