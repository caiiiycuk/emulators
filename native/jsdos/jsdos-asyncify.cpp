//
// Created by caiiiycuk on 13.11.2019.
//
#include <jsdos-asyncify.h>

#ifdef EMSCRIPTEN
// clang-format off
#include <emscripten.h>

EM_JS(void, syncSleep, (), {
    if (!Module.sync_sleep) {
      throw new Error("Async environment does not exists");
      return;
    }

    if (Asyncify.state === 0 && (Date.now() - Module.last_wakeup < 16 /* 60 FPS */)) {
      return;
    }

    if (Asyncify.state === 0) {
      Module.sleep_started_at = Date.now();
    }

    return Asyncify
      .handleSleep(function(wakeUp) { 
        if (Asyncify.currData) {
          Asyncify.whenDone().catch(Module.uncaughtAsyncify);
        }
        Module.sync_sleep(() => {
          ++Module.sleep_count;
          Module.sleep_time += Date.now() - Module.sleep_started_at;
          delete Module.sleep_started_at;
          Module.last_wakeup = Date.now();

          wakeUp();
        }); 
      })
  });

EM_JS(bool, initTimeoutSyncSleep, (), {
    Module.alive = true;
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
      Module.destroyAsyncify();
      Module.uncaught(error);
    };

    return true;
  });

EM_JS(bool, initMessageSyncSleep, (bool worker), {
    Module.alive = true;
    Module.sleep_count = 0;
    Module.sleep_time = 0;
    Module.last_wakeup = Date.now();
    Module.sync_sleep = function(wakeUp) {
      if (Module.sync_wakeUp) {
        throw new Error("Trying to sleep in sleeping state!");
        return;  // already sleeping
      }

      Module.sync_wakeUp = wakeUp;
      
      function postWakeUpMessage() {
        if (worker) {
          postMessage({name : "ws-sync-sleep", props: { sessionId : Module.sessionId } });
        } else {
          window.postMessage({name : "ws-sync-sleep", props: { sessionId : Module.sessionId } },
                              "*");
        }
      }

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
        var wakeUp = Module.sync_wakeUp;
        delete Module.sync_wakeUp;

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

extern "C" void asyncify_sleep(unsigned int ms) {
#ifdef EMSCRIPTEN
  if (asyncifyLockCount == 0) {
    syncSleep();
  }
#else
  while (paused) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  if (ms == 0) {
    return;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}
