#include <jsdos-support.h>

#ifndef EMSCRIPTEN
#include <atomic>
#endif

namespace {
#ifndef EMSCRIPTEN
    std::atomic_bool _exit(false);
#else
    bool _exit = false;
#endif
}

void jsdos::init() {
    _exit = false;
}

bool jsdos::isExitRequested() {
    return _exit;
}

void jsdos::requestExit() {
    _exit = true;
}
