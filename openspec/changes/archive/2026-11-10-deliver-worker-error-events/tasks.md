# Tasks

## 1. Deliver Worker errors

- [x] 1.1 Update `worker.onerror` in `src/dos/dosbox/ts/worker.ts` to send `ws-err` with `sessionId`, `tag: "worker"`, and `message` including `filename` when available; `grep -rn filename src test` confirms that nobody reads the old `type` and `filename` fields.
- [x] 1.2 Update `worker errors preserve the live audio resources and error message` in `test/src/test-audio-lifecycle.ts`: an `error` event on the fixture Worker with `message` and `filename` produces exactly one `onMessage("error", ...)` through the real `CommandInterfaceOverTransportLayer`, with both values in the text. Audio and the worker remain intact; after `ci.exit()` and `ws-exit`, terminate, close, and onExit counts are each 1. Verification: `yarn test:node --grep "worker errors"` passes and fails before the fix in 1.1.
- [x] 1.3 Add the error-before-`ws-server-ready` scenario: emit `error` before `initMessageHandler`, then create CI and verify that the message is delivered from the queue. Verification: the same grep passes.
- [x] 1.4 Add an English `CHANGELOG.md` entry under Fixed for the current unpublished version. Verification: `yarn run eslint src --ext ts,tsx --max-warnings 0`, `yarn run eslint test --ext ts,tsx --max-warnings 0`, and `yarn test:node:build` pass.

## 2. Integration

- [x] 2.1 `yarn test:browser` passes; extend the existing native test in `test/src/browser-audio-lifecycle.ts` (a synthetic `ErrorEvent` on a real Worker) to expect a second `error` message, `[worker]fixture recoverable worker error`, for worker backends.
