// Minimal host/GL substitutes. The runner includes actual production functions.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>
#include <cstdio>
#include <stdexcept>
using UINT8 = uint8_t;
using UINT16 = uint16_t;
using UINT32 = uint32_t;
using INT8 = int8_t;
using INT32 = int32_t;
using INT64 = int64_t;
using Bitu = unsigned;
using rgb_t = UINT32;
using GLuint = unsigned;
#define INLINE inline
#define TEXMODE_FORMAT(x) (((x) >> 8) & 15)
#define TEXMODE_NCC_TABLE_SELECT(x) (((x) >> 5) & 1)
#define TEXMODE_SEQ_8_DOWNLD(x) (((x) >> 31) & 1)
#define TEXMODE_CLAMP_S(x) (((x) >> 6) & 1)
#define TEXMODE_CLAMP_T(x) (((x) >> 7) & 1)
#define TEXLOD_TDATA_SWIZZLE(x) (((x) >> 25) & 1)
#define TEXLOD_TDATA_SWAP(x) (((x) >> 26) & 1)
#define TEXLOD_TDIRECT_WRITE(x) (((x) >> 27) & 1)
#define FLIPENDIAN_INT32(x) __builtin_bswap32(x)
#define BYTE4_XOR_LE(x) (x)
#define BYTE_XOR_LE(x) (x)
#define LOG(...) [](const char*, ...) {}
#define LOG_TEXTURE_RAM false
#define CLAMP(x, lo, hi) ((x) = (x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
#define MAKE_ARGB(a, r, g, b) ((UINT32(a) << 24) | ((r) << 16) | ((g) << 8) | (b))
void E_Exit(const char*) { throw std::runtime_error("Unexpected E_Exit"); }
enum { textureMode, tLOD };
struct reg_t { UINT32 u = 0; };
struct ncc_table {
    reg_t reg[12];
    rgb_t *palette = nullptr, *palettea = nullptr;
    int y[16] = {}, ir[4] = {}, ig[4] = {}, ib[4] = {};
    int qr[4] = {}, qg[4] = {}, qb[4] = {};
    rgb_t texel[256] = {};
    bool dirty = false;
};
struct tmu_state {
    reg_t reg[2];
    UINT8 *ram = nullptr;
    UINT32 mask = 0, lodoffset[9] = {}, wmask = 0, hmask = 0, lodmask = 511;
    INT32 lodmin = 0;
    INT64 dsdx = 0, dtdx = 0, dsdy = 0, dtdy = 0;
    bool regdirty = false;
    ncc_table ncc[2];
    rgb_t palette[256] = {}, *lookup = nullptr, *texel[16] = {};
};
struct voodoo_state { tmu_state tmu[2]; UINT32 chipmask = 6; bool ogl = true, active = true; };
voodoo_state state, *v = &state;
void recompute_texture_params(tmu_state *t) {
    t->lookup = t->ncc[TEXMODE_NCC_TABLE_SELECT(t->reg[textureMode].u)].texel;
    t->regdirty = false;
}
INT64 fast_reciplog(INT64, INT32 *lod) { *lod = 0; return 0; }
struct poly_extra_data {
    voodoo_state *state;
    UINT32 r_textureMode0, r_textureMode1, texcount;
};
struct ogl_texmap {
    bool valid_pal;
    UINT32 format, current_id;
    std::map<const UINT32, GLuint> *ids;
    UINT32 width, height, ilod, lodmask, ncc_table;
};
struct ogl_texture_data { GLuint texID; bool enable; };
std::map<const UINT32, ogl_texmap> textures[2];
UINT32 texrgb[256 * 256], ogl_texture_index = 1;
unsigned uploads = 0, deletes = 0, flushes = 0;
void VOGL_ClearBeginMode() { ++flushes; }
void glDeleteTextures(int, GLuint*) { ++deletes; }
void glGenTextures(int, GLuint *id) { static GLuint next = 1; *id = next++; }
void glBindTexture(int, GLuint) {}
void glTexParameteri(int, int, int) {}
void glTexImage2D(int, int, int, int, int, int, int, int, const void*) { ++uploads; }
using PFNGLGENERATEMIPMAPEXTPROC = void (*)(int);
void mipmap(int) {}
PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT = mipmap, db_glGenerateMipmapEXT = mipmap;
enum { GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR,
       GL_TEXTURE_MAG_FILTER, GL_LINEAR, GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T,
       GL_CLAMP_TO_EDGE, GL_REPEAT, GL_RGBA, GL_BGRA_EXT, GL_UNSIGNED_INT_8_8_8_8_REV };
void voodoo_ogl_texture_clear(UINT32, int);
#include "voodoo-functions.inc"

static void reset() {
    for (int t = 0; t < 2; ++t) {
        for (auto &entry : textures[t]) ogl_destroy_cached_texture(entry.second);
        textures[t].clear();
        texture_dirty[t].pending = false;
    }
    state = voodoo_state{};
    palette_changed = false;
    uploads = deletes = flushes = 0;
}

int main() {
    // Actual texture_w, with an allocation ending exactly at the TMU boundary.
    for (int format : {0, 10}) {
        reset();
        std::vector<UINT8> ram(64);
        auto &t = v->tmu[0];
        t.ram = ram.data(); t.mask = 63; t.reg[textureMode].u = format << 8;
        t.lodoffset[8] = 62;
        texture_w(8 << 15, 0x44332211);
        assert(ram[62] == 0x11 && ram[63] == 0x22 && ram[0] == 0x33 && ram[1] == 0x44);
    }
    reset();
    v->tmu[0].mask = v->tmu[1].mask = 0x1fffff;
    textures[0][0x100] = {true, 0, 1, nullptr, 16, 16, 0, 511, 0};
    textures[0][0x40000] = {true, 0, 2, nullptr, 16, 16, 0, 511, 0};
    textures[1][0x100] = {true, 0, 3, nullptr, 16, 16, 0, 511, 0};
    for (unsigned i = 0; i < 32768; ++i) voodoo_ogl_texture_clear(0x100 + i * 4, 0);
    assert(deletes == 0 && flushes == 0 && textures[0].size() == 2);
    ogl_flush_texture_writes(0);
    assert(deletes == 1 && textures[0].count(0x40000) && textures[1].size() == 1);
    ogl_flush_texture_writes(0);
    assert(deletes == 1);
    // A texture that wraps RAM, and a write that wraps RAM.
    textures[0][0x1ffffe] = {true, 0, 4, nullptr, 4, 1, 0, 511, 0};
    voodoo_ogl_texture_clear(0, 0); ogl_flush_texture_writes(0);
    assert(!textures[0].count(0x1ffffe));
    voodoo_ogl_texture_clear(0x1ffffe, 0); ogl_flush_texture_writes(0);
    assert(textures[0].empty());

    // Both NCC formats: rewrite Y/I/Q without texel downloads or regdirty.
    for (unsigned format : {1u, 9u}) {
        reset();
        std::vector<UINT8> ram(64);
        auto &t = v->tmu[0];
        t.ram = ram.data(); t.mask = 63; t.reg[textureMode].u = format << 8;
        t.lookup = t.ncc[0].texel;
        poly_extra_data extra{v, format << 8, 0, 1};
        ogl_texture_data td[2] = {};
        ncc_table_write(&t.ncc[0], 0, 0x20202020);
        prepare_tmu(&t);
        assert(!t.ncc[0].dirty && palette_changed);
        ogl_cache_texture(&extra, td);
        const GLuint initial = td[0].texID;
        const UINT32 old_color = texrgb[0];
        const UINT32 initial_sum = calculate_palsum(0, format << 8);
        for (auto reg : {0u, 4u, 8u}) {
            palette_changed = false;
            ncc_table_write(&t.ncc[0], reg, reg == 0 ? 0x40404040 : 1);
            assert(palette_changed && !t.regdirty);
            prepare_tmu(&t);
            voodoo_ogl_invalidate_paltex();
            ogl_cache_texture(&extra, td);
            assert(td[0].texID != initial && texrgb[0] != old_color);
            assert(calculate_palsum(0, format << 8) != initial_sum);
        }
        assert(uploads == 4);
        // Revert the table: reuse the original uploaded palette variant.
        ncc_table_write(&t.ncc[0], 0, 0x20202020);
        ncc_table_write(&t.ncc[0], 4, 0);
        ncc_table_write(&t.ncc[0], 8, 0);
        prepare_tmu(&t); voodoo_ogl_invalidate_paltex(); ogl_cache_texture(&extra, td);
        assert(td[0].texID == initial && uploads == 4);
        // The other NCC table has independent colors.
        ncc_table_write(&t.ncc[1], 0, 0x60606060);
        extra.r_textureMode0 |= 1 << 5;
        t.reg[textureMode].u = extra.r_textureMode0;
        t.regdirty = true;
        prepare_tmu(&t); ogl_cache_texture(&extra, td);
        assert(uploads == 5 && td[0].texID != initial);
    }
    reset();
    std::puts("RAM wrap, deferred invalidation, TMU isolation, NCC updates/reuse: passed");
}
