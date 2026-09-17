import { readFileSync } from "fs";
import { resolve } from "path";
import { Page } from "playwright";

export async function runWorkerWebGL(page: Page) {
    const source = readFileSync(resolve(__dirname, "../../src/dos/dosbox/cpp/worker-protocol.cpp"), "utf8");
    const start = source.indexOf("          const gl = Module.canvas.getContext");
    const end = source.indexOf("        })();", start);
    if (start < 0 || end < 0) {
        throw new Error("Worker WebGL initialization was not found");
    }

    // Exercise the EM_JS implementation itself, including the fallback without VAOs.
    await page.evaluate((body) => {
        function check(condition: boolean, message: string) {
            if (!condition) throw new Error(message);
        }
        for (const mode of ["default VAO", "custom VAO", "no VAO"]) {
            const canvas = document.createElement("canvas");
            canvas.width = canvas.height = 4;
            const gl = canvas.getContext("webgl", { alpha: false, depth: false, stencil: false })!;
            check(gl !== null, "WebGL is unavailable");
            const vao = gl.getExtension("OES_vertex_array_object");
            check(vao !== null, "VAO test coverage requires OES_vertex_array_object");
            if (mode === "no VAO") {
                const getExtension = gl.getExtension.bind(gl);
                gl.getExtension = (name: string) => name === "OES_vertex_array_object" ? null : getExtension(name);
            }
            let renderFrame = () => {/**/};
            const module: any = { canvas, HEAPU8: new Uint8Array(4 * 4 * 3) };
            new Function("Module", "requestAnimationFrame", body)(module, (callback: () => void) => {
                renderFrame = callback;
                return 1;
            });
            module.glfx = true;
            const texture0 = gl.createTexture();
            const texture1 = gl.createTexture();
            gl.activeTexture(gl.TEXTURE0);
            gl.bindTexture(gl.TEXTURE_2D, texture0);
            gl.activeTexture(gl.TEXTURE1);
            gl.bindTexture(gl.TEXTURE_2D, texture1);
            module.bind3Dfx(4, 4);
            check(gl.getParameter(gl.TEXTURE_BINDING_2D) === texture1, "FBO setup changed texture binding");
            gl.clearColor(1, 0, 0, 1);
            gl.clear(gl.COLOR_BUFFER_BIT);

            if (mode === "custom VAO") vao!.bindVertexArrayOES(vao!.createVertexArrayOES());
            for (let index = 0; index < 2; ++index) {
                gl.bindBuffer(gl.ARRAY_BUFFER, gl.createBuffer());
                gl.bufferData(gl.ARRAY_BUFFER, new Uint8Array(128), gl.STATIC_DRAW);
                gl.vertexAttribPointer(index, 4, gl.UNSIGNED_BYTE, true, 8, 4);
                if (index === 0) gl.enableVertexAttribArray(index);
                else gl.disableVertexAttribArray(index);
            }
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, gl.createBuffer());
            gl.bindBuffer(gl.ARRAY_BUFFER, gl.createBuffer());
            gl.useProgram(null);
            const capabilities = [gl.BLEND, gl.DEPTH_TEST, gl.STENCIL_TEST, gl.SCISSOR_TEST, gl.CULL_FACE];
            capabilities.forEach((capability) => gl.enable(capability));
            gl.blendFunc(gl.ZERO, gl.ZERO);
            gl.depthFunc(gl.NEVER);
            gl.stencilFunc(gl.NEVER, 0, 0xff);
            gl.scissor(0, 0, 0, 0);
            gl.cullFace(gl.FRONT_AND_BACK);
            gl.colorMask(false, true, false, false);

            function snapshot() {
                const values: any[] = [
                    gl.getParameter(gl.CURRENT_PROGRAM), gl.getParameter(gl.ACTIVE_TEXTURE),
                    gl.getParameter(gl.TEXTURE_BINDING_2D), gl.getParameter(gl.ARRAY_BUFFER_BINDING),
                    gl.getParameter(gl.ELEMENT_ARRAY_BUFFER_BINDING), gl.getParameter(gl.FRAMEBUFFER_BINDING),
                    gl.getParameter(vao!.VERTEX_ARRAY_BINDING_OES),
                    ...gl.getParameter(gl.COLOR_WRITEMASK),
                    ...capabilities.map((capability) => gl.isEnabled(capability)),
                ];
                for (let index = 0; index < 2; ++index) {
                    for (const parameter of [gl.VERTEX_ATTRIB_ARRAY_ENABLED, gl.VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,
                        gl.VERTEX_ATTRIB_ARRAY_SIZE, gl.VERTEX_ATTRIB_ARRAY_TYPE, gl.VERTEX_ATTRIB_ARRAY_NORMALIZED,
                        gl.VERTEX_ATTRIB_ARRAY_STRIDE]) {
                        values.push(gl.getVertexAttrib(index, parameter));
                    }
                    values.push(gl.getVertexAttribOffset(index, gl.VERTEX_ATTRIB_ARRAY_POINTER));
                }
                const active = gl.getParameter(gl.ACTIVE_TEXTURE);
                gl.activeTexture(gl.TEXTURE0);
                values.push(gl.getParameter(gl.TEXTURE_BINDING_2D));
                gl.activeTexture(active);
                return values;
            }
            function checkState(expected: any[]) {
                const actual = snapshot();
                check(actual.every((value, index) => value === expected[index]), mode + ": state changed");
                check(gl.getError() === gl.NO_ERROR, mode + ": WebGL error");
            }
            function checkPixel(red: number, green: number) {
                gl.bindFramebuffer(gl.FRAMEBUFFER, null);
                const pixels = new Uint8Array(4 * 4 * 4);
                gl.readPixels(0, 0, 4, 4, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
                gl.bindFramebuffer(gl.FRAMEBUFFER, module.fbo);
                for (let i = 0; i < pixels.length; i += 4) {
                    check(pixels[i] === red && pixels[i + 1] === green && pixels[i + 2] === 0,
                        mode + ": incorrect output pixel at " + i);
                }
            }
            for (const active of [gl.TEXTURE0, gl.TEXTURE1]) {
                gl.activeTexture(active);
                const expected = snapshot();
                module.swapbuffers();
                checkState(expected);
                checkPixel(255, 0);
            }

            const expected = snapshot();
            for (let i = 1; i < module.HEAPU8.length; i += 3) module.HEAPU8[i] = 255;
            module.updateTexture(0, 4, 4);
            renderFrame();
            checkState(expected);
            checkPixel(0, 255);

            const drawArrays = gl.drawArrays;
            const failure = new Error("draw failure");
            gl.drawArrays = () => {
                throw failure;
            };
            let caught: unknown;
            try {
                module.swapbuffers();
            } catch (error) {
                caught = error;
            } finally {
                gl.drawArrays = drawArrays;
            }
            check(caught === failure, "Draw failure was swallowed");
            checkState(expected);
            gl.getExtension("WEBGL_lose_context")!.loseContext();
        }
    }, source.slice(start, end));
    console.log("Worker WebGL state and pixel checks passed (default VAO, custom VAO, no VAO).");
}
