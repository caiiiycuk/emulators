# Verification of deliver-worker-error-events

## Result

All 5 tasks are complete. Verification was performed on top of commit `eb19cf3`. At verification time, this change was uncommitted and had not been published or archived.

`worker.onerror` sends the existing `ws-err` message with `sessionId`, `tag: "worker"`, and the event text. A nonempty `filename` is appended in parentheses. Protocol session validation is not weakened. An error message does not shut down audio or the transport.

## Specification checks

- "Worker reports an error after startup": `worker errors preserve audio and deliver live messages` uses the real `CommandInterfaceOverTransportLayer`. After `ws-server-ready`, a Worker event delivers exactly one `error` message with text `[worker]fixture worker failure (fixture-worker.js)`.
- "Worker error before the backend is ready": `worker errors preserve audio and deliver queued messages` raises the error before CI creation and `initMessageHandler`. The message passes through both queues and is delivered on subscription without a filename. A subsequent `ws-server-ready` calls ready with `null`, so the error does not become an initialization failure.
- "Exit after a Worker error": both tests verify that the context, port, and node remain open, the Worker is not terminated, and gesture handlers remain registered until explicit exit. After `ci.exit()` and a `ws-exit` response, the context, port, and node are released once, the Worker is terminated once, and `onExit` fires once. Repeated exit is safe.

The existing error-during-audio-creation test still verifies successful backend installation and preservation of the late-created context. Exit-after-error tests with `audioWorklet=false` and `true` also pass.

Browser checks cover `dosboxDirect`, `dosboxWorker`, `dosboxXDirect`, and `dosboxXWorker` using production files and real AudioContext, AudioWorkletNode, and Worker instances. In worker modes, a synthetic ErrorEvent on a real Worker now produces a second message, `[worker]fixture recoverable worker error`. The context stays running until exit; suspended-stop, running-stop, and remount checks pass. A fatal Worker process crash was not simulated.

## Checks and results

- Before the handler fix, `yarn test:node --grep "worker errors"` reproduced the defect: the subscriber received an empty list instead of an error message. Log: `/tmp/opencode/worker-error-events-red.log`.
- After the fix, the same command passed all 5 tests.
- `yarn run eslint src --ext ts,tsx --max-warnings 0` and its `test` equivalent passed, including a repeat after the production build.
- `yarn run tsc --noEmit` passed.
- The additional strict browser helper compilation passed: `yarn run tsc --noEmit --target esnext --module commonjs --moduleResolution node --strict --skipLibCheck --esModuleInterop test/src/browser-audio-lifecycle.ts`.
- `yarn test:node:build` passed all 85 tests. Log: `/tmp/opencode/worker-error-events-node-build.log`.
- `yarn run gulp production` passed. Log: `/tmp/opencode/worker-error-events-production.log`.
- `yarn test:browser` passed, including all four native audio lifecycle checks and the rest of the standard suite. Log: `/tmp/opencode/worker-error-events-browser.log`.
- `git diff --check` passed.
- `openspec validate deliver-worker-error-events --strict --json` passed without issues; `openspec instructions apply` reported `all_done`, with 5/5 tasks complete.

The production build updated `src/build.ts` through the normal build process: `Build.version` is `8.5.3 (b86e52a18fe0c57fd7089c6bea2535be)` and `buildSeed` is `1794312299255`. The package version did not change, and no new npm archive was created.

## Scope boundaries

A `filename` search in `src/` and `test/` found no readers of the old Worker error fields except the replaced raw-message test. The public API, protocol, direct backend, AudioWorklet/DSP, and C/C++ are unchanged. An English entry was added to Fixed for version 8.5.3 in `CHANGELOG.md`.

`test:browser:net` and other browsers were not run. `talks/` and the existing `.idea/` in the `native/dosbox-x` submodule were not touched.
