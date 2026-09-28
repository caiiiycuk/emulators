//
// Created by Alexander Guryanov on 15/11/22.
//
#include <config.h>
#include <jsdos-events.h>
#include <jsdos-timer.h>
#include <protocol.h>
#include <render.h>
#include <stdlib.h>

#include <cstdlib>
#include <list>
#include <string>
#include <vector>

#include "../dosbox/include/cpu.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
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

#ifdef C_DEBUG
extern Bitu DEBUG_EnableDebugger(void);
#endif

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
#ifdef C_DEBUG
  static bool altPressed = false;
  static bool f12Pressed = false;
#endif

  while (executeNextKeyEventAt <= frameTime && it != keyEvents.end()) {
    auto key = it->key;
    auto pressed = it->pressed;

#ifdef C_DEBUG
    if (key == KBD_leftalt) {
      altPressed = pressed;
    }
    if (key == KBD_f12) {
      f12Pressed = pressed;
    }
#endif

    KEYBOARD_AddKey(key, pressed);
    it = keyEvents.erase(it);
    if (it != keyEvents.end()) {
      executeNextKeyEventAt = frameTime + (it->clientTime - clientTime);
      clientTime = it->clientTime;
    } else {
      executeNextKeyEventAt = frameTime + 16;
    }
  }

#ifdef C_DEBUG
  if (altPressed && f12Pressed) {
    DEBUG_EnableDebugger();
  }
#endif
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

#ifndef EMSCRIPTEN
std::mutex triggerMutex;
#endif
std::vector<std::string> triggerEvents;
extern void IpxNetStartServer();
extern "C" void EMSCRIPTEN_KEEPALIVE TriggerEventByName(const char* name) {
#ifndef EMSCRIPTEN
  std::lock_guard<std::mutex> g(triggerMutex);
#endif
  triggerEvents.push_back(name);
}

#ifdef JSDOS_X
void MAPPER_TriggerEventByName(const std::string& name);
extern void RDTSC_rebase();
extern uint32_t emulator_speed;
#endif

extern bool ticksLocked;

void jsdos::handleTriggeredEvents() {
#ifndef EMSCRIPTEN
  std::lock_guard<std::mutex> g(triggerMutex);
#endif

  for (auto& next: triggerEvents) {
    if (next == "hand_ipx_startserver") {
      IpxNetStartServer();
    } else if (next.find("fast_forward:") == 0) {
      ticksLocked = next.back() == '1';
    } else if (next.find("frame_skip:") == 0) {
      render.frameskip.max = atoi(&next.back());
    } else if (next.find("auto_adjust:") == 0) {
      CPU_CycleAutoAdjust = next.back() == '1';
    } else if (next.find("cycles:") == 0) {
      CPU_CycleMax = atoi(next.substr(7).c_str());
#ifdef JSDOS_X
      RDTSC_rebase();
#endif
    } else if (next.find("speed:") == 0) {
#ifdef JSDOS_X
      emulator_speed = atoi(next.substr(6).c_str());
#endif
    }
#if JSDOS_X
    else {
      MAPPER_TriggerEventByName(next);
    }
#else
    else {
      printf("ERR! Event '%s' is not supported by dosbox backend\n", next.c_str());
    }
#endif
  }
  triggerEvents.clear();
}
