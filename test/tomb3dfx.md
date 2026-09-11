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

## Rendering regression check

After saving both final screenshots, the scenario checks a 200×100 region of
snow floor to Lara's right. At least 75% must be visible grey snow pixels. The
previous defective frame had 0% coverage; the corrected frame has about 98%.
This catches the missing-texture regression without requiring an exact match
of Lara's animation or using a damaged frame as a golden image. It is a targeted
check, not a validation of all Voodoo rendering.

## Verified fix (2026-09-10)

The browser renderer in `native/jsdos/jsdos-voodoo.cpp` had the old texture cache
implementation, while the shared texture RAM writer had already been changed
to invalidate by write address. Its cache still only looked for an exact base
address. The browser implementation now invalidates overlapping cached textures
and recreates entries when an address is reused with another texture layout.
These changes match the existing local DOSBox-X texture-cache changes.

Visual inspection confirmed wall and snow textures in the first level before
F4, with Lara stationary and without waiting for the demo. They remain present
after two F4 presses. The scenario now fails if the floor regresses to the dark
teal rendering; both final screenshots are retained on that failure. There were
no page errors or failed requests in the corrected run.

## Native cache regressions

```sh
python3 test/native/voodoo-regression.py
```

This requires g++ with AddressSanitizer. The harness compiles the actual TMU
write, NCC update, and GL texture-cache functions from both renderer sources
against small RAM and GL substitutes. It checks circular 8/16-bit writes,
deferred invalidation after bulk writes, TMU isolation, and NCC color updates
and palette-variant reuse without texel downloads. It also compares the shared
cache implementations to detect divergence between native and js-dos builds.

Texture writes now accumulate a conservative dirty byte interval per TMU; cache
entries are checked once on the next cache access, after pending GL primitives
are completed. Wrapped writes may invalidate extra entries but cannot leave a
stale texture. Every RAM write index wraps independently. NCC updates invalidate
palette-dependent textures, and the selected NCC texel table participates in
the palette checksum.
