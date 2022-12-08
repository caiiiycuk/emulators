#ifndef JSDOS_RUNTIME_H_
#define JSDOS_RUNTIME_H_

namespace jsdos {
    void init();
    bool isExitRequested();
    void requestExit();
    void cout(const char* data, int amount);
}

#endif
