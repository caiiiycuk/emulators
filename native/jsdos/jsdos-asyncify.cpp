//
// Created by caiiiycuk on 13.11.2019.
//
#include "./include/jsdos-asyncify.h"

#include <cmath>
#include <string>

#include "cpu.h"
#include "render.h"

#ifdef EMSCRIPTEN
// clang-format off
#include <emscripten.h>

#if JSPI
EM_ASYNC_JS(void, syncSleep, (unsigned int ms, bool nonSkippable), {
    if (!Module.sync_sleep) {
      throw new Error("Async environment does not exists");
      return;
    }

    const now = Date.now();
    if (!nonSkippable && (now - Module.last_wakeup) < 24 /* 30 FPS */) {
      return;
    }

    if (nonSkippable) {
      Module.wakeUpAt = Date.now() + ms;
      ++Module.nonskippable_sleep_count;
    }

    ++Module.sleep_count;
    Module.sleep_started_at = now;

    return new Promise((resolve) => {
      Module.sync_sleep(() => {
        Module.sleep_time += now - Module.sleep_started_at;
        Module.last_wakeup = now;
        resolve();
      });
    });
});
#else
EM_JS(void, syncSleep, (unsigned int ms, bool nonSkippable), {
    if (!Module.sync_sleep) {
      throw new Error("Async environment does not exists");
      return;
    }

    const now = Date.now();
    if (Asyncify.state === 0) { // NORMAL
      if (!nonSkippable && (now - Module.last_wakeup) < 24 /* 30 FPS */) {
        return;
      }
      
      if (nonSkippable) {
        Module.wakeUpAt = Date.now() + ms;
        ++Module.nonskippable_sleep_count;
      }

      ++Module.sleep_count;
      
      Module.sleep_started_at = now;
    } else if (Asyncify.state === 2) { // REWIND
      Module.sleep_time += now - Module.sleep_started_at;
      Module.last_wakeup = now;

      if (Asyncify.asyncPromiseHandlers === null) {
        Asyncify.whenDone().catch(Module.uncaughtAsyncify);
      }
    }

    Asyncify.handleSleep(Module.sync_sleep);
  });
#endif

EM_JS(bool, initTimeoutSyncSleep, (), {
    Module.alive = true;
    Module.nonskippable_sleep_count = 0;
    Module.sleep_count = 0;
    Module.sleep_time = 0;
    Module.last_wakeup = Date.now();
    Module.sync_sleep = function(wakeUp) {
      setTimeout(function() {
          if (!Module.alive) {
            return;
          }

          if (Module.paused === true) {
            var checkIntervalId = setInterval(function() {
              if (Module.paused === false) {
                clearInterval(checkIntervalId);
                wakeUp();
              }
            }, 16);
          } else {
            wakeUp();
          } 
        });
    };

    Module.destroyAsyncify = function() {
      Module.alive = false;
      delete Module.sync_sleep;
    };
    Module.uncaughtAsyncify = function(error) {
      console.error(error);
      Module.destroyAsyncify();
      Module.uncaught(error);
    };

    return true;
  });

EM_JS(bool, initMessageSyncSleep, (bool worker), {
    Module.alive = true;
    Module.nonskippable_sleep_count = 0;
    Module.sleep_count = 0;
    Module.sleep_time = 0;
    Module.last_wakeup = Date.now();
    
    function postWakeUpMessage() {
      if (worker) {
        postMessage({name : "ws-sync-sleep", props: { sessionId : Module.sessionId } });
      } else {
        window.postMessage({name : "ws-sync-sleep", props: { sessionId : Module.sessionId } },
                            "*");
      }
    }

    Module.sync_sleep = function(wakeUp) {
      if (Module.sync_wakeUp) {
        throw new Error("Trying to sleep in sleeping state!");
        return;  // already sleeping
      }

      Module.sync_wakeUp = wakeUp;
      
      if (Module.paused === true) {
        var checkIntervalId = setInterval(function() {
          if (Module.paused === false) {
            clearInterval(checkIntervalId);
            postWakeUpMessage();
          }
        }, 16);
      } else {
        postWakeUpMessage();
      }
    };

    Module.receive = function(ev) {
      var data = ev.data;
      if (ev.data.name === "wc-sync-sleep" &&
          Module.sessionId === ev.data.props.sessionId) {
        if (Module.wakeUpAt !== undefined && Date.now() < Module.wakeUpAt) {
          postWakeUpMessage();
          return;
        }
        var wakeUp = Module.sync_wakeUp;
        delete Module.sync_wakeUp;
        delete Module.wakeUpAt;

        if (Module.alive) {
          wakeUp();
        }
      }
    };

    if (worker) {
      self.addEventListener("message", Module.receive, { passive: true });
    } else {
      window.addEventListener("message", Module.receive, { passive: true });
    }

    Module.destroyAsyncify = function() {
      if (worker) {
        self.removeEventListener("message", Module.receive);
      } else {
        window.removeEventListener("message", Module.receive);
      }

      Module.alive = false;
      delete Module.sync_sleep;
    };
    Module.uncaughtAsyncify = function(error) {
      console.error(error);
      Module.destroyAsyncify();
      Module.uncaught(error);
    };

    return true;
  });

EM_JS(void, destroyAsyncify, (), {
    Module.destroyAsyncify();
  });

EM_JS(bool, isWorker, (), {
    return typeof importScripts === "function";
  });

EM_JS(bool, isNode, (), {
    return typeof process === "object" && typeof process.versions === "object" && typeof process.versions.node === "string";
  });

// clang-format on
#else
#include <thread>
#endif

volatile bool paused = false;

void server_pause() {
  paused = true;
#ifdef EMSCRIPTEN
  EM_ASM(({
      Module.paused = true;
  }));
#endif
}

void server_resume() {
  paused = false;
#ifdef EMSCRIPTEN
  EM_ASM(({
      Module.paused = false;
  }));
#endif
}

void jsdos::initAsyncify() {
#ifdef EMSCRIPTEN
  if (isNode()) {
    initTimeoutSyncSleep();
  } else {
    initMessageSyncSleep(isWorker());
  }
#endif
}

void jsdos::destroyAsyncify() {
#ifdef EMSCRIPTEN
    ::destroyAsyncify();
#endif
}

int asyncifyLockCount = 0;
void jsdos::asyncifyLock() {
  ++asyncifyLockCount;
}

void jsdos::asyncifyUnlock() {
  --asyncifyLockCount;
}

extern "C" void asyncify_sleep(unsigned int ms, bool nonSkippable) {
  if (asyncifyLockCount != 0) {
    return;
  }
#ifdef EMSCRIPTEN
  syncSleep(ms, nonSkippable);
#else
  while (paused) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  if (ms == 0 || !nonSkippable) {
    return;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}


std::string cpuMetrics = "";
double cpuMax = 0;
double increaseTicksCount = 0;
bool fastForward = false;
double emulatorSpeed = 0;
double cpuPercUsed = 0;
bool cpuSockdrive = false;
bool cpuAuto = false;
bool cpuSkip = false;
double frameskip = 0;

extern bool ticksLocked;
extern uint32_t emulator_speed;
extern bool wasSockdriveRead;

void clearTicks() {
  cpuMax = 0;
  increaseTicksCount = 0;
  fastForward = false;
  emulatorSpeed = 0;
  cpuPercUsed = 0;
  cpuSockdrive = false;
  cpuAuto = false;
  cpuSkip = false;
  frameskip = 0;
}

void jsdos::increaseticks() {
  cpuMax += CPU_CycleMax;
#ifdef JSDOS_X
  emulatorSpeed += emulator_speed;
#endif
  cpuPercUsed += CPU_CyclePercUsed;

  increaseTicksCount++;

  if (ticksLocked) {
    fastForward = true;
  }

#ifdef JSDOS_X
  if (wasSockdriveRead) {
    cpuSockdrive = true;
  }
#endif

  if (CPU_CycleAutoAdjust) {
    cpuAuto = true;
  }

  if (CPU_SkipCycleAutoAdjust) {
    cpuSkip = true;
  }

  frameskip =+ render.frameskip.max;
}

extern "C" const char* EMSCRIPTEN_KEEPALIVE getCPUMetrics() {
  static std::string copy;
  if (increaseTicksCount > 0) {
    copy =
      std::to_string((int32_t) std::round(cpuMax / increaseTicksCount)) + "|" +
      std::to_string((int32_t) std::round(emulatorSpeed / increaseTicksCount)) + "|" +
      std::to_string((int32_t) std::round(cpuPercUsed / increaseTicksCount)) + "|" +
      std::to_string((int32_t) std::round(frameskip / increaseTicksCount)) + "|" +
      (fastForward ? "1" : "0") + (cpuSockdrive ? "1" : "0") + (cpuAuto ? "1" : "0") + (cpuSkip ? "1" : "0") + " " +
      cpuMetrics;
    cpuMetrics = "";
    clearTicks();
    return copy.c_str();
  } else {
    return "";
  }
}