"""Compile the real TMU/cache functions with a small RAM/GL test harness.

Run: python3 test/native/voodoo-regression.py
Requires a C++11 compiler with AddressSanitizer. No emulator or GL context needed.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
hardware = root / "native/dosbox-x/src/hardware"
emu = (hardware / "voodoo_emu.cpp").read_text()


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index("\n}", start) + 2] + "\n"


reference = None
for renderer in [hardware / "voodoo_opengl.cpp", root / "native/jsdos/jsdos-voodoo.cpp"]:
    gl = renderer.read_text()
    code = "static bool palette_changed = false;\n"
    for signature in ["void ncc_table_write(", "void ncc_table_update(",
                      "INLINE INT32 prepare_tmu(", "INT32 texture_w("]:
        code += function(emu, signature)
    start = gl.index("static UINT32 crc_32_tab[]")
    code += gl[start:gl.index("static bool ogl_palette_format", start)]
    for signature in ["static bool ogl_palette_format(", "UINT32 calculate_palsum(",
                      "static void ogl_destroy_cached_texture("]:
        code += function(gl, signature)
    start = gl.index("struct ogl_dirty_range")
    code += gl[start:gl.index("static void ogl_flush_texture_writes", start)]
    for signature in ["static void ogl_flush_texture_writes(", "void ogl_cache_texture(",
                      "void voodoo_ogl_texture_clear(", "void voodoo_ogl_invalidate_paltex("]:
        code += function(gl, signature)
    normalized = code.replace("db_gl", "gl")
    if reference is None:
        reference = normalized
    else:
        assert normalized == reference, "Native and js-dos TMU/cache implementations diverged"
    with tempfile.TemporaryDirectory(prefix="voodoo-regression-") as temp:
        directory = Path(temp)
        (directory / "voodoo-functions.inc").write_text(code)
        binary = directory / "test"
        subprocess.run(["g++", "-std=c++11", "-fsanitize=address", "-g", "-I", temp,
                        str(root / "test/native/voodoo-regression.cpp"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("Passed:", renderer.relative_to(root), flush=True)
