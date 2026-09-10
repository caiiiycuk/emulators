# Tomb Raider 3dfx diagnostic (issue #440)

Build local artifacts and run the separate scenario:

```sh
yarn gulp
yarn test:browser:tomb3dfx
```

If running `yarn test:node:build` too, run it **before** `yarn gulp`: the Node
build copies raw WASM loaders into `dist`, and the full build appends the worker
bootstrap required by browser scenarios.

The ordinary `yarn test:browser` does not run this game. The scenario uses
`dosboxXWorker` with an OffscreenCanvas, the bundle's unmodified configuration,
and a 640×480 canvas screenshot at device scale 1. No remote game resources
or public API changes are needed.

The supplied `hoffmeister.li_js-dos_games_TOMB3DFX.jsdos` is copied unchanged to
`test/dosbox-x/tomb3dfx.jsdos`. SHA-256:
`1beda976efeff11f3550b89dff3e0e9a25490cfd4afd5e9c3c65596b6b5696a4`.

The runner recognizes the title screen, presses Enter to open the passport,
recognizes its open pages, presses Enter again, and recognizes the level's
health bar. Screen recognition uses small color regions, not reference images
of damaged textures. Each key press has a matching release. After the camera
settles, it captures the level without moving Lara, then presses F4 twice with
rendering delays between presses. It enters the game before the idle demo.

Artifacts are overwritten on the next diagnostic run in
`dist/test-artifacts/tomb3dfx/`, and survive `yarn gulp`:

- `level-before-f4.png`, `level-between-f4.png`, `level-after-f4.png`.
- Boot frames, recognized menu/passport/level frames, and the last waiting frame
  for each stage.
- `keys.jsonl`: timestamped key press/release requests.
- `browser.log.json`: browser console, page errors, and failed HTTP requests.
- On failure, `failure.png` (when the canvas is available) and `failure.txt`.

The overall scenario timeout is 180 seconds, with shorter per-screen waits.
For a forced timeout check:

```sh
BROWSER_TEST_TIMEOUT_MS=12000 yarn test:browser:tomb3dfx
```

This command should exit nonzero, retain diagnostics, and close Chromium and
the HTTP server. Copy any results worth keeping before another run.

## Observed result (2026-09-10)

The rebuilt local emulator entered the first level before the demo. Visual
inspection confirmed dark, missing-looking surface textures before F4. Two
separate F4 presses caused video mode resets in the log but did not restore
those textures. Both final screenshots show Lara stationary in the initial
room. There were no page errors or failed requests.

A successful run means the diagnostic screenshots were produced; it does not
assert that rendering is correct. No golden image is generated. Use the same
entry sequence and inspect `level-before-f4.png` when evaluating future renderer
changes; the F4 workaround was not confirmed on this build.
