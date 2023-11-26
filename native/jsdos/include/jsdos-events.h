//
// Created by Alexander Guryanov on 15/11/22.
//

#ifndef JS_DOS_JSDOS_EVENTS_H
#define JS_DOS_JSDOS_EVENTS_H

#include <cstdint>

namespace jsdos {
    void DoKeyEvents();
    void DoMouseEvents();
    void Mouse_CursorMoved(float xrel,float yrel,float x,float y,bool emulate);
    void Mouse_ButtonPressed(uint8_t button);
    void Mouse_ButtonReleased(uint8_t button);
}

#endif  // JS_DOS_JSDOS_EVENTS_H
