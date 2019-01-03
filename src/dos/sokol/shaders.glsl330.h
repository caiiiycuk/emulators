#pragma once
/*
    #version:1# (machine generated, don't edit!)

    Generated by sokol-shdc (https://github.com/floooh/sokol-tools)

    Overview:

        Shader program 'display':
            Get shader desc: display_shader_desc()
            Vertex shader: display_vs
                Attribute slots:
                    ATTR_display_vs_in_pos = 0
                    ATTR_display_vs_in_uv = 1
            Fragment shader: display_fs
                Image 'tex':
                    Type: SG_IMAGETYPE_2D
                    Component Type: SG_SAMPLERTYPE_FLOAT
                    Bind slot: SLOT_tex = 0


    Shader descriptor structs:

        sg_shader display = sg_make_shader(display_shader_desc());

    Vertex attribute locations for vertex shader 'display_vs':

        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .layout = {
                .attrs = {
                    [ATTR_display_vs_in_pos] = { ... },
                    [ATTR_display_vs_in_uv] = { ... },
                },
            },
            ...});

    Image bind slots, use as index in sg_bindings.vs_images[] or .fs_images[]

        SLOT_tex = 0;

*/
#include <stdint.h>
#include <stdbool.h>
#if !defined(SOKOL_SHDC_ALIGN)
  #if defined(_MSC_VER)
    #define SOKOL_SHDC_ALIGN(a) __declspec(align(a))
  #else
    #define SOKOL_SHDC_ALIGN(a) __attribute__((aligned(a)))
  #endif
#endif
#define ATTR_display_vs_in_pos (0)
#define ATTR_display_vs_in_uv (1)
#define SLOT_tex (0)
/*
    #version 330
    
    layout(location = 0) in vec2 in_pos;
    out vec2 uv;
    layout(location = 1) in vec2 in_uv;
    
    void main()
    {
        gl_Position = vec4((in_pos * 2.0) - vec2(1.0), 0.5, 1.0);
        uv = in_uv;
    }
    
*/
static const char display_vs_source_glsl330[197] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x33,0x30,0x0a,0x0a,0x6c,0x61,
    0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,
    0x30,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x69,0x6e,0x5f,0x70,0x6f,
    0x73,0x3b,0x0a,0x6f,0x75,0x74,0x20,0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x3b,0x0a,
    0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,
    0x3d,0x20,0x31,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x69,0x6e,0x5f,
    0x75,0x76,0x3b,0x0a,0x0a,0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,
    0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x67,0x6c,0x5f,0x50,0x6f,0x73,0x69,0x74,0x69,
    0x6f,0x6e,0x20,0x3d,0x20,0x76,0x65,0x63,0x34,0x28,0x28,0x69,0x6e,0x5f,0x70,0x6f,
    0x73,0x20,0x2a,0x20,0x32,0x2e,0x30,0x29,0x20,0x2d,0x20,0x76,0x65,0x63,0x32,0x28,
    0x31,0x2e,0x30,0x29,0x2c,0x20,0x30,0x2e,0x35,0x2c,0x20,0x31,0x2e,0x30,0x29,0x3b,
    0x0a,0x20,0x20,0x20,0x20,0x75,0x76,0x20,0x3d,0x20,0x69,0x6e,0x5f,0x75,0x76,0x3b,
    0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    #version 330
    
    uniform sampler2D tex;
    
    layout(location = 0) out vec4 frag_color;
    in vec2 uv;
    
    void main()
    {
        frag_color = vec4(texture(tex, uv).xyz, 1.0);
    }
    
*/
static const char display_fs_source_glsl330[161] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x33,0x30,0x0a,0x0a,0x75,0x6e,
    0x69,0x66,0x6f,0x72,0x6d,0x20,0x73,0x61,0x6d,0x70,0x6c,0x65,0x72,0x32,0x44,0x20,
    0x74,0x65,0x78,0x3b,0x0a,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,
    0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x30,0x29,0x20,0x6f,0x75,0x74,0x20,0x76,
    0x65,0x63,0x34,0x20,0x66,0x72,0x61,0x67,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,
    0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x3b,0x0a,0x0a,0x76,0x6f,0x69,
    0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x66,
    0x72,0x61,0x67,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x20,0x3d,0x20,0x76,0x65,0x63,0x34,
    0x28,0x74,0x65,0x78,0x74,0x75,0x72,0x65,0x28,0x74,0x65,0x78,0x2c,0x20,0x75,0x76,
    0x29,0x2e,0x78,0x79,0x7a,0x2c,0x20,0x31,0x2e,0x30,0x29,0x3b,0x0a,0x7d,0x0a,0x0a,
    0x00,
};
#if !defined(SOKOL_GFX_INCLUDED)
  #error "Please include sokol_gfx.h before shaders.glsl330.h"
#endif
static inline const sg_shader_desc* display_shader_desc(void) {
  if (sg_query_backend() == SG_BACKEND_GLCORE33) {
    static sg_shader_desc desc;
    static bool valid;
    if (!valid) {
      valid = true;
      desc.attrs[0].name = "in_pos";
      desc.attrs[1].name = "in_uv";
      desc.vs.source = display_vs_source_glsl330;
      desc.vs.entry = "main";
      desc.fs.source = display_fs_source_glsl330;
      desc.fs.entry = "main";
      desc.fs.images[0].name = "tex";
      desc.fs.images[0].type = SG_IMAGETYPE_2D;
      desc.fs.images[0].sampler_type = SG_SAMPLERTYPE_FLOAT;
      desc.label = "display_shader";
    };
    return &desc;
  }
  return 0;
}
