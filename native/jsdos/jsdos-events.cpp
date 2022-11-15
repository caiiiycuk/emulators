//
// Created by Alexander Guryanov on 15/11/22.
//
#include <list>
#include <protocol.h>

#include <jsdos-timer.h>

struct KeyEvent {
  KBD_KEYS key;
  bool pressed;
  uint64_t clientTime;
};

std::list<KeyEvent> keyEvents;
double executeNextKeyEventAt = 0;

void DoKeyEvents() {
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

void server_add_key(KBD_KEYS key, bool pressed, uint64_t pressedMs) {
  keyEvents.push_back({ key, pressed, pressedMs });
  if (keyEvents.size() == 1 && pressed) {
    executeNextKeyEventAt = GetMsPassedFromStart();
  }
}
