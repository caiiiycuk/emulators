//
// Created by Alexander Guryanov on 15/11/22.
//
#include <list>
#include <protocol.h>

#include <jsdos-events.h>
#include <jsdos-timer.h>
#include <cstdlib>

#ifndef EMSCRIPTEN
#include <mutex>

std::mutex keyMutex;
std::mutex mouseMutex;
#endif

void Mouse_CursorMoved(float xrel, float yrel, float x, float y, bool emulate);
void Mouse_CursorSet(float x, float y);
void Mouse_ButtonPressed(uint8_t button);
void Mouse_ButtonReleased(uint8_t button);

struct KeyEvent {
  KBD_KEYS key;
  bool pressed;
  uint64_t clientTime;
};

struct MouseEvent {
  uint8_t method; // 0 - motion, 1 - pressed, 2 - released
  float xrel;
  float yrel;
  float x;
  float y;
  bool emulate;
  uint8_t button;
};

std::list<KeyEvent> keyEvents;
std::list<MouseEvent> mouseEvents;
double executeNextKeyEventAt = 0;

void jsdos::DoKeyEvents() {
#ifndef EMSCRIPTEN
  std::lock_guard<std::mutex> g(keyMutex);
#endif

  if (keyEvents.empty()) {
    return;
  }

  auto frameTime = GetMsPassedFromStart();
  auto it = keyEvents.begin();
  auto clientTime = it->clientTime;

  while (executeNextKeyEventAt <= frameTime && it != keyEvents.end()) {
    auto key = it->key;
    auto pressed = it->pressed;

    KEYBOARD_AddKey(key, pressed);
    it = keyEvents.erase(it);
    if (it != keyEvents.end()) {
      executeNextKeyEventAt = frameTime + (it->clientTime - clientTime);
      clientTime = it->clientTime;
    } else {
      executeNextKeyEventAt = frameTime + 16;
    }
  }
}

void jsdos::DoMouseEvents() {
#ifndef EMSCRIPTEN
  std::lock_guard<std::mutex> g(mouseMutex);
#endif
  for (auto &next: mouseEvents) {
    switch (next.method) {
      case 0: ::Mouse_CursorMoved(next.xrel, next.yrel, next.x, next.y, next.emulate); break;
      case 1: ::Mouse_ButtonPressed(next.button); break;
      case 2: ::Mouse_ButtonReleased(next.button); break;
      default: abort();
    }
  }
  mouseEvents.clear();
}

void jsdos::Mouse_CursorMoved(float xrel,float yrel,float x,float y,bool emulate) {
#ifndef EMSCRIPTEN
  std::lock_guard<std::mutex> g(mouseMutex);
#endif
  mouseEvents.push_back({ 0, xrel, yrel, x, y, emulate, 0 });
}

void jsdos::Mouse_ButtonPressed(uint8_t button) {
#ifndef EMSCRIPTEN
  std::lock_guard<std::mutex> g(mouseMutex);
#endif
  mouseEvents.push_back({ 1, 0, 0, 0, 0, false, button });
}

void jsdos::Mouse_ButtonReleased(uint8_t button) {
#ifndef EMSCRIPTEN
  std::lock_guard<std::mutex> g(mouseMutex);
#endif
  mouseEvents.push_back({ 2, 0, 0, 0, 0, false, button });
}

void server_add_key(KBD_KEYS key, bool pressed, uint64_t pressedMs) {
#ifndef EMSCRIPTEN
  std::lock_guard<std::mutex> g(keyMutex);
#endif

  keyEvents.push_back({ key, pressed, pressedMs });
  if (keyEvents.size() == 1 && pressed) {
    executeNextKeyEventAt = GetMsPassedFromStart();
  }
}
