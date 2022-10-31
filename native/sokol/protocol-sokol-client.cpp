//
// Created by Alexander Guryanov on 31/10/22.
//

#include "protocol-client-glue.h"

#define SOKOL_NO_ENTRY
#define SOKOL_IMPL
#define SOKOL_GLCORE33

#include "../sokol-lib/sokol_app.h"
#include "../sokol-lib/sokol_audio.h"
#include "../sokol-lib/sokol_gfx.h"

#include <protocol.h>


#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

extern double GetMsPassedFromStart();

void client_run() {
    sapp_desc appDescription = {};
    appDescription.init_cb = []() { sokolInit(); };

    appDescription.frame_cb = []() { sokolFrame(); };

    appDescription.cleanup_cb = []() { server_exit(); };

    appDescription.event_cb = [](const sapp_event *event) {
        static float prevX = 0;
        static float prevY = 0;

        switch (event->type) {
            case SAPP_EVENTTYPE_KEY_DOWN:
            case SAPP_EVENTTYPE_KEY_UP:
                sokolKeyEvent(event);
                break;
            case SAPP_EVENTTYPE_MOUSE_MOVE: {
                server_mouse_moved(event->mouse_x / WINDOW_WIDTH,
                                   event->mouse_y / WINDOW_HEIGHT,
                                   false,
                                   GetMsPassedFromStart());
            } break;
            case SAPP_EVENTTYPE_MOUSE_UP:
            case SAPP_EVENTTYPE_MOUSE_DOWN: {
                server_mouse_moved(event->mouse_x / WINDOW_WIDTH,
                                   event->mouse_y / WINDOW_HEIGHT,
                                   false,
                                   GetMsPassedFromStart());
                server_mouse_button(event->mouse_button, event->type == SAPP_EVENTTYPE_MOUSE_DOWN, GetMsPassedFromStart());
            } break;
            default:;
        }
    };

    appDescription.width = WINDOW_WIDTH;
    appDescription.height = WINDOW_HEIGHT;
    appDescription.ios_keyboard_resizes_canvas = false;
    appDescription.gl_force_gles2 = true;
    appDescription.html5_ask_leave_site = false;
    appDescription.html5_canvas_resize = true;

    sapp_run(&appDescription);
}
