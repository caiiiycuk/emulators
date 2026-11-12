# Proposal

## Why

`src/dos/dosbox/ts/worker.ts` sends `ws-err` from `Worker.onerror` without `sessionId` or `tag`. `CommandInterfaceOverTransportLayer.onServerMessage` rejects it at the `props.sessionId !== this.transport.sessionId` check in `src/protocol/protocol.ts`. Worker script loading or execution errors, such as a broken `wdosbox.js`, a CSP violation, or an exception outside the message handler, never reach the consumer through `ci.events().onMessage("error", ...)`. The consumer sees only stalled initialization or an unresponsive emulator.

## What Changes

- The `ws-err` message produced by `Worker.onerror` includes the transport's `sessionId` and a `tag`, passes `CommandInterface` validation, and uses the same `[tag]message` format as backend errors.
- The `ErrorEvent` details are carried in `message` rather than separate `type` and `filename` fields, preserving the `ws-err` contract.
- A Node test uses the real `CommandInterfaceOverTransportLayer` to verify that `Worker.onerror` produces `onMessage("error", ...)` without affecting audio, the worker, or `exit`. The behavior from `fix-audio-worklet-lifecycle` is preserved.

Unchanged: the `ws-*`/`wc-*` protocol, public API, `direct.ts` (which has no `onerror`), C/C++, submodules, and the `CommandInterface` response to `ws-err` (notification only, not shutdown).

## Capabilities

### New Capabilities
- `worker-error-events`: deliver errors from the Worker itself (`error` events on the `Worker` object) to the `CommandInterface` consumer as error messages without changing the transport lifecycle.

### Modified Capabilities
<!-- None: openspec/specs/ had no existing specifications when this change was proposed. -->

## Impact

- `src/dos/dosbox/ts/worker.ts`: the `worker.onerror` handler.
- `test/src/test-audio-lifecycle.ts`: update the existing `worker errors preserve the live audio resources and error message` test, which checks a raw message without `sessionId`, and add coverage through the real CI.
- `CHANGELOG.md`: add a Fixed entry for the current unpublished version.
- Consumers such as js-dos will receive previously missing `error` messages. This is not a breaking change because `onMessage` already receives backend errors.
