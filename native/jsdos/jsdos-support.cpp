#include <protocol.h>
#include <jsdos-timer.h>
#include <jsdos-support.h>
#include <string>
//#include <sstream>

#ifndef EMSCRIPTEN
#include <atomic>
#endif

namespace {
#ifndef EMSCRIPTEN
    std::atomic_bool jsdos_exit(false);
#else
    bool jsdos_exit = false;
#endif
}

void jsdos::init() {
    jsdos_exit = false;
}

bool jsdos::isExitRequested() {
    return jsdos_exit;
}

void jsdos::requestExit() {
    jsdos_exit = true;
}

void jsdos::cout(const char* data, int amount) {
  static std::string line;
  static double time[2] = {0.0, 0.0};
  static int timeIndex = 0;
  static bool reportRuns = false;
  static double runs = 5000;

  if (reportRuns) {
    reportRuns = false;
    auto dtime = time[1] - time[0];
    runs = runs * 2;
    
// TODO @caiiiycuk: in dosbox-x data always have 0 runs 0 secs    
//    std::istringstream stream(std::string(data, amount));
//    stream >> runs;

    std::string message = "dhry2: " + std::to_string((long)runs) + " " +
                          std::to_string(dtime) + " " +
                          std::to_string(runs * 1000 / dtime / 1757);

    client_stdout(message.c_str(), message.length());
    return;
  }

  if (amount >= 7 && data[0] == '~' && data[1] == '>' && data[2] == 'd' &&
      data[3] == 't' && data[4] == 'i' && data[5] == 'm' && data[6] == 'e') {
    time[timeIndex] = GetMsPassedFromStart();
    timeIndex = (timeIndex + 1) % 2;
    reportRuns = timeIndex == 0;
  } else {
    for (int i = 0; i < amount; ++i) {
      char next = data[i];
      if (isascii(next) || next == '\n') {
        line += next;
      }

      if (next == '\n') {
        client_stdout(line.c_str(), line.length());
        line.clear();
      }
    }
  }
}
