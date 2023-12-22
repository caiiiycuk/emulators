//
// Created by Alexander Guryanov on 08/12/22.
//

#include <string>

#include "config.h"
#include "logging.h"
#include "render.h"
#include "sdlmain.h"

extern int aspect_ratio_x, aspect_ratio_y;
extern void E_Exit(const char * format,...);
extern Bitu MakeAspectTable(Bitu skip, Bitu height, double scaley, Bitu miny);
extern void RENDER_CallBack(GFX_CallBackFunctions_t function);
extern void RENDER_FinishLineHandler(const void *s);

void RENDER_Reset(void) {
  Bitu width = render.src.width;
  Bitu height = render.src.height;
  bool dblw = render.src.dblw;
  bool dblh = render.src.dblh;

  double gfx_scalew;
  double gfx_scaleh;

  if (width == 0 || height == 0) return;

  Bitu gfx_flags, xscale, yscale;
  ScalerSimpleBlock_t *simpleBlock = &ScaleNormal1x;
  
  gfx_scalew = 1;
  gfx_scaleh = 1;

  if (render.aspect == ASPECT_TRUE && !render.aspectOffload) {
    if (render.src.ratio > 1.0) {
      gfx_scalew = 1;
      gfx_scaleh = render.src.ratio;
    } else {
      gfx_scalew = (1.0 / render.src.ratio);
      gfx_scaleh = 1;
    }
  }

  if (sdl.desktop.isperfect) /* Handle scaling if no pixel-perfect mode */
    goto forcenormal;

  if ((dblh && dblw) || (render.scale.forced && dblh == dblw/*this branch works best with equal scaling in both directions*/)) {
    /* Initialize always working defaults */
    if (render.scale.size == 2)
      simpleBlock = &ScaleNormal2x;
    else if (render.scale.size == 3)
      simpleBlock = &ScaleNormal3x;
    else if (render.scale.size == 1 && !(dblh || dblw) && render.scale.hardware)
      simpleBlock = &ScaleNormal1x;
    else if (render.scale.size == 4 && !(dblh || dblw) && render.scale.hardware)
      simpleBlock = &ScaleNormal2x;
    else if (render.scale.size == 6 && !(dblh || dblw) && render.scale.hardware)
      simpleBlock = &ScaleNormal3x;
    else if (render.scale.size == 4 && !render.scale.hardware)
      simpleBlock = &ScaleNormal4x;
    else if (render.scale.size == 5 && !render.scale.hardware)
      simpleBlock = &ScaleNormal5x;
    else if (render.scale.size == 8 && !(dblh || dblw) && render.scale.hardware)
      simpleBlock = &ScaleNormal4x;
    else if (render.scale.size == 10 && !(dblh || dblw) &&
             render.scale.hardware)
      simpleBlock = &ScaleNormal5x;
  } else if (dblw && !render.scale.hardware) {
    if (render.scale.forced && render.scale.size >= 2)
      simpleBlock = &ScaleNormal2xDw;
    else
      simpleBlock = &ScaleNormalDw;
  } else if (dblh && !render.scale.hardware) {
    if (render.scale.forced && render.scale.size >= 2)
      simpleBlock = &ScaleNormal2xDh;
    else
      simpleBlock = &ScaleNormalDh;
  }
  
  if (simpleBlock == NULL) {
  forcenormal:
    simpleBlock = &ScaleNormal1x;
  }
  
  gfx_flags = simpleBlock->gfxFlags;
  xscale = simpleBlock->xscale;
  yscale = simpleBlock->yscale;
  // LOG_MSG("Scaler:%s",simpleBlock->name);
  
  switch (render.src.bpp) {
    case 8:
      render.src.start = (render.src.width * 1) / sizeof(Bitu);
      if (gfx_flags & GFX_CAN_8)
        gfx_flags |= GFX_LOVE_8;
      else
        gfx_flags |= GFX_LOVE_32;
      break;
    case 15:
      render.src.start = (render.src.width * 2) / sizeof(Bitu);
      gfx_flags |= GFX_LOVE_15;
      gfx_flags = (gfx_flags & ~GFX_CAN_8) | GFX_RGBONLY;
      break;
    case 16:
      render.src.start = (render.src.width * 2) / sizeof(Bitu);
      gfx_flags |= GFX_LOVE_16;
      gfx_flags = (gfx_flags & ~GFX_CAN_8) | GFX_RGBONLY;
      break;
    case 32:
      render.src.start = (render.src.width * 4) / sizeof(Bitu);
      gfx_flags |= GFX_LOVE_32;
      gfx_flags = (gfx_flags & ~GFX_CAN_8) | GFX_RGBONLY;
      break;
    default:
      render.src.start = (render.src.width * 1) / sizeof(Bitu);
      if (gfx_flags & GFX_CAN_8)
        gfx_flags |= GFX_LOVE_8;
      else
        gfx_flags |= GFX_LOVE_32;
      break;
  }
  gfx_flags &= ~GFX_SCALING;
  gfx_flags |= GFX_RGBONLY | GFX_CAN_RANDOM;
  if (!gfx_flags) {
    if (simpleBlock == &ScaleNormal1x)
      E_Exit("Failed to create a rendering output");
    else
      goto forcenormal;
  }
  width *= xscale;
  constexpr Bitu skip = 0;
  if (gfx_flags & GFX_SCALING) {
    if (render.scale.size == 1 && render.scale.hardware) {  // hardware_none
      if (dblh) gfx_scaleh *= 1;
      if (dblw) gfx_scalew *= 1;
    } else if (render.scale.size == 4 && render.scale.hardware) {
      if (dblh) gfx_scaleh *= 2;
      if (dblw) gfx_scalew *= 2;
    } else if (render.scale.size == 6 && render.scale.hardware) {
      if (dblh && dblw) {
        gfx_scaleh *= 3;
        gfx_scalew *= 3;
      } else if (dblh) {
        gfx_scaleh *= 2;
      } else if (dblw)
        gfx_scalew *= 2;
    } else if (render.scale.size == 8 && render.scale.hardware) {  // hardware4x
      if (dblh) gfx_scaleh *= 4;
      if (dblw) gfx_scalew *= 4;
    } else if (render.scale.size == 10 &&
               render.scale.hardware) {  // hardware5x
      if (dblh && dblw) {
        gfx_scaleh *= 5;
        gfx_scalew *= 5;
      } else if (dblh) {
        gfx_scaleh *= 4;
      } else if (dblw)
        gfx_scalew *= 4;
    }
    height = MakeAspectTable(skip, render.src.height, (double)yscale, yscale);
  } else {
    // Print a warning when hardware scalers are selected, hopefully the first
    // video mode will not have dblh or dblw or AR will be wrong
    if (render.scale.hardware) {
      LOG_MSG(
          "Output does not support hardware scaling, switching to normal "
          "scalers");
      render.scale.hardware = false;
    }
    if ((gfx_flags & GFX_CAN_RANDOM) && gfx_scaleh > 1) {
      gfx_scaleh *= yscale;
      height = MakeAspectTable(skip, render.src.height, gfx_scaleh, yscale);
    } else {
      gfx_flags &= ~GFX_CAN_RANDOM;  // Hardware surface when possible
      height = MakeAspectTable(skip, render.src.height, (double)yscale, yscale);
    }
  }
  /* update the aspect ratio */
  sdl.srcAspect.x = aspect_ratio_x > 0
                        ? aspect_ratio_x
                        : (int)(render.src.width * (render.src.dblw ? 2 : 1));
  sdl.srcAspect.y =
      aspect_ratio_y > 0
          ? aspect_ratio_y
          : (int)floor((render.src.height * (render.src.dblh ? 2 : 1) *
                        render.src.ratio) +
                       0.5);
  sdl.srcAspect.xToY = (double)sdl.srcAspect.x / sdl.srcAspect.y;
  sdl.srcAspect.yToX = (double)sdl.srcAspect.y / sdl.srcAspect.x;
  LOG_MSG("Aspect ratio: %u x %u  xToY=%.3f yToX=%.3f", sdl.srcAspect.x,
          sdl.srcAspect.y, sdl.srcAspect.xToY, sdl.srcAspect.yToX);
  
  gfx_flags = GFX_SetSize(width, height, gfx_flags, gfx_scalew, gfx_scaleh,
                          &RENDER_CallBack);
  
  if (gfx_flags & GFX_CAN_8)
    render.scale.outMode = scalerMode8;
  else if (gfx_flags & GFX_CAN_15)
    render.scale.outMode = scalerMode15;
  else if (gfx_flags & GFX_CAN_16)
    render.scale.outMode = scalerMode16;
  else if (gfx_flags & GFX_CAN_32)
    render.scale.outMode = scalerMode32;
  else
    E_Exit("Failed to create a rendering output");
  ScalerLineBlock_t *lineBlock;
  if (gfx_flags & GFX_HARDWARE) {
      render.scale.complexHandler = 0;
      lineBlock = &simpleBlock->Linear;
  } else {
      render.scale.complexHandler = 0;
      lineBlock = &simpleBlock->Random;
  }
  switch (render.src.bpp) {
    case 8:
      render.scale.lineHandler = (*lineBlock)[0][render.scale.outMode];
      render.scale.linePalHandler = (*lineBlock)[4][render.scale.outMode];
      render.scale.inMode = scalerMode8;
      render.scale.cachePitch = render.src.width * 1;
      break;
    case 15:
      render.scale.lineHandler = (*lineBlock)[1][render.scale.outMode];
      render.scale.linePalHandler = 0;
      render.scale.inMode = scalerMode15;
      render.scale.cachePitch = render.src.width * 2;
      break;
    case 16:
      render.scale.lineHandler = (*lineBlock)[2][render.scale.outMode];
      render.scale.linePalHandler = 0;
      render.scale.inMode = scalerMode16;
      render.scale.cachePitch = render.src.width * 2;
      break;
    case 32:
      render.scale.lineHandler = (*lineBlock)[3][render.scale.outMode];
      render.scale.linePalHandler = 0;
      render.scale.inMode = scalerMode32;
      render.scale.cachePitch = render.src.width * 4;
      break;
    default:
      // render.src.bpp=8;
      render.scale.lineHandler = (*lineBlock)[0][render.scale.outMode];
      render.scale.linePalHandler = (*lineBlock)[4][render.scale.outMode];
      render.scale.inMode = scalerMode8;
      render.scale.cachePitch = render.src.width * 1;
      break;
      // E_Exit("RENDER:Wrong source bpp %d", render.src.bpp );
  }
  render.scale.blocks = render.src.width / SCALER_BLOCKSIZE;
  render.scale.lastBlock = render.src.width % SCALER_BLOCKSIZE;
  render.scale.inHeight = render.src.height;
  /* Reset the palette change detection to it's initial value */
  render.pal.first = 0;
  render.pal.last = 255;
  render.pal.changed = false;
  memset(render.pal.modified, 0, sizeof(render.pal.modified));
  // Finish this frame using a copy only handler
  RENDER_DrawLine = RENDER_FinishLineHandler;
  render.scale.outWrite = 0;
  /* Signal the next frame to first reinit the cache */
  render.scale.clearCache = true;

  if (!sdl.window_too_small) render.active = true;

  last_gfx_flags = gfx_flags;
  
  // Ensure VMware mouse support knows the current parameters
  VMWARE_ScreenParams(sdl.clip.x, sdl.clip.y, sdl.clip.w, sdl.clip.h,
                      sdl.desktop.fullscreen);
#if defined(MACOSX) && !defined(C_SDL2)
  SetWindowTransparency(-1);
#endif
}
