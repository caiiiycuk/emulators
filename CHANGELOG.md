# Changelog

## v8.5.1

### Added

- Added a Civilization II sockdrive browser test (`yarn test:browser:civ2`) with a separate CI workflow and README badge. The test is included in `test:all`.

### Fixed

- Fixed sockdrive drives hanging Windows 95 guests (for example Civilization II) after the DOSBox-X update: the sockdrive disk size is now passed to `imageDisk` in bytes instead of KiB.
- Fixed the browser DOSBox-X debugger crashing with `RuntimeError: unreachable` on MCP `RUN`/`VRT` by adding `DEBUG_Loop` to the asyncify list.
- Fixed missing `gl4es` build dependencies for `libdosbox-x-jsdos` and `wdosbox-x-jspi`.

## v8.5.0

### Added

- Added a DOSBox-X debug worker build with browser MCP/debugger support through `dosboxXDebugWorker` and the `mcpServerPort` backend option.
- Added browser diagnostics and CI coverage for D3DTunnel 3Dfx/WebGL rendering.
- Added worker WebGL state preservation tests and audio worklet regression tests.

### Improved

- Updated DOSBox-X from `2025.05.03-patch-10-gdd065da31` to `dosbox-x-v2026.08.31-92-g48abd08ee` ([compare](https://github.com/js-dos/dosbox-x/compare/dd065da3111b6dd00abb70820fb750bf36a8d5c2...48abd08ee956aeb786308c6091fd28a328514cc3)) and refreshed the asyncify list.
- Improved browser audio playback when emulation speed changes, including sample-rate handling, pitch stability, underrun recovery, rebuffering, and fade-in/fade-out behavior.
- Improved DOSBox-X mixer output pacing and buffering, including DC bias correction support.
- Improved WebGL frame blits for 3Dfx rendering by preserving GL state and avoiding framebuffer feedback loops.
- Improved Sokol audio output by waiting for available audio buffer space before pushing samples.

### Fixed

- Fixed Voodoo/OpenGL resize handling in DOSBox-X.
- Fixed stale frame updates after frame size changes.
- Fixed out-of-bounds RGBA frame writes in the Sokol protocol path.
- Fixed D3DTunnel browser rendering stability, including spline transition tolerance.
- Fixed sockdrive code in the DOSBox-X update branch.

### Changed

- Split regular and debug DOSBox-X builds so debugger sources are only included in debug builds.
- Removed the obsolete js-dos DOSBox-X render shim and routed frame updates through the worker protocol path.
- Added the `test:browser:d3dtunnel` script and included Tomb Raider 3Dfx and D3DTunnel checks in `test:all`.

## v8.4.1

- Improve sockdrive in direct mode
- Fix sockdrive in node environment
- Fix for #caiiiycuk/js-dos/303
- Fix for #caiiiycuk/js-dos/310
- Fix for #caiiiycuk/js-dos/426

## v8.4.0

### Added

- Added full 3Dfx acceleration support in the browser through WebGL-backed rendering.
- Added a new browser networking layer powered by [WebRTC-NET](https://github.com/caiiiycuk/WebRTC-NET), enabling multiplayer/network-capable DOS experiences through WebRTC transport.
- Added improved sockdrive support for streamed disk images and range-based loading.

### Improved

- Improved sockdrive range loading reliability. Failed range requests now report an error after the final retry instead of leaving the emulator waiting indefinitely.
- Improved the JavaScript/WebAssembly build toolchain by updating to Emscripten SDK 5.0.2.
- Updated the JavaScript build environment to Node.js 22.x.

### Changed

- Removed the custom Binaryen override from the JavaScript build workflow. The build now uses the Binaryen version bundled with the selected Emscripten SDK.
- Simplified the WebAssembly build setup, making builds easier to reproduce with the standard Emscripten toolchain.
