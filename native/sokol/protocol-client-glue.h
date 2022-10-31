//
// Created by Alexander Guryanov on 31/10/22.
//

#ifndef JS_DOS_PROTOCOL_GLUE_H
#define JS_DOS_PROTOCOL_GLUE_H

#include "../sokol-lib/sokol_app.h"

void sokolInit();
void sokolFrame();
void sokolKeyEvent(const sapp_event *event);

#endif  // JS_DOS_PROTOCOL_GLUE_H
