export function isJspiSupported() {
    const wasm = typeof WebAssembly === "object" ? WebAssembly as any : undefined;
    return typeof wasm?.Suspending === "function" && typeof wasm?.promising === "function";
}
