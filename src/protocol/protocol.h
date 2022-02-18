//
// Created by caiiiycuk on 27.02.2020.
//

#ifndef JS_DOS_JS_DOS_PROTOCOL_H
#define JS_DOS_JS_DOS_PROTOCOL_H

#include <keyboard.h>
#include <cstdint>

enum NetworkType {
  NETWORK_NA = -1,
  NETWORK_DOSBOX_IPX = 0,
};

// -- Client (Web Page)

void client_frame_set_size(int width, int height);
void client_frame_update_lines(uint32_t* lines, uint32_t count, void* rgba);
void client_sound_init(int freq);
void client_sound_push(const float* samples, int num_samples);
void client_stdout(const char* data, uint32_t amount);

void client_log(const char* tag, const char* message);
void client_warn(const char* tag, const char* message);
void client_error(const char* tag, const char* message);

void client_network_connected(NetworkType networkType, const char* address, uint32_t port);
void client_network_disconnected(NetworkType networkType);

// -- Server (Worker)

extern int server_run();
extern void server_add_key(KBD_KEYS key, bool pressed, uint64_t pressedMs);
extern void server_mouse_moved(float x, float y, bool relative, uint64_t movedMs);
extern void server_mouse_button(int button, bool pressed, uint64_t pressedMs);
extern void server_mouse_sync(uint64_t syncMs);
extern void server_pause();
extern void server_resume();
extern void server_mute();
extern void server_unmute();
extern void server_exit();

extern void server_network_connect(NetworkType networkType, const char* address, uint32_t port);
extern void server_network_disconnect(NetworkType networkType);

#ifndef EMSCRIPTEN
extern void server_loop();
#endif

#endif  // JS_DOS_JS_DOS_PROTOCOL_H
