# Changelog

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
