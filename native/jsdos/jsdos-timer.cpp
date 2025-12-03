//
// Created by caiiiycuk on 28.02.2020.
//

#include <timer.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#include <thread>
#endif

#include <jsdos-timer.h>
#include <jsdos-asyncify.h>

void jsdos::initTimer() {
}

double now() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000. + ts.tv_nsec / 1000000.;
}

double GetMsPassedFromStart() {
    static double startedAt = now();
    return now() - startedAt;
}

mstime GetTicks() {
    return GetMsPassedFromStart();
}