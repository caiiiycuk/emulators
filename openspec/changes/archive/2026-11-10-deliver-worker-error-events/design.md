# Design

## Context

See proposal.md, Why. The following facts were checked in `src/protocol/protocol.ts`:

- `onServerMessage` discards messages with `props.sessionId !== transport.sessionId` before the `switch`.
- `ws-err` calls `onErr(props.tag, props.message)`, then `fireMessage("error", "[" + tag + "]" + message)`. Errors with `tag === "panic"` also accumulate in `panicMessages` and reject initialization at `ws-server-ready`.
- The backend (`src/dos/dosbox/cpp/worker-protocol.cpp`) always adds `sessionId` to `props`; `worker.onerror` in `worker.ts` is the only source of `ws-err` without it.
- Messages sent before `initMessageHandler` accumulate in `MessagesQueue` and are delivered when a handler is registered.

## Goals / Non-Goals

**Goals:**
- Messages from `Worker.onerror` pass validation and notify the consumer.

**Non-Goals:**
- Automatic shutdown or restart after a Worker error.
- Adding fields to the `ws-err` format or changing `direct.ts` or C/C++.

## Decisions

1. Use `tag: "worker"`, not `"panic"`. A `panic` before `ws-server-ready` rejects initialization, while a Worker `error` event does not prove that the backend has stopped. This was verified in `fix-audio-worklet-lifecycle`: an `onmessage` handler can throw while the Worker continues running. The consumer decides how to respond to the text.
2. Carry error details in `message` (`e.message` plus `" (" + e.filename + ")"` when `filename` is nonempty), and remove the separate `type` and `filename` fields. `onErr` only reads `tag` and `message`; check `filename` references in `src/` and `test/` to confirm that no consumer reads the extra fields.
3. Take `sessionId` from the `dosWorker` function parameter, which also supplies `transportLayer.sessionId`.

Rejected alternative: fill in a missing `sessionId` for every `ws-err` in `onServerMessage`. This would weaken session validation for all such messages.

## Risks / Trade-offs

- [The `worker errors preserve the live audio resources and error message` test compares the raw `{type, filename, message}` message] → update its expectation and also verify delivery through the real `CommandInterfaceOverTransportLayer`.
- [Consumers will see new `error` messages] → the `[worker]...` format matches the existing `[tag]message` format, so there is no breaking change.
