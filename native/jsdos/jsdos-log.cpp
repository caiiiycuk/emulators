//
// Created by caiiiycuk on 13.04.2020.
//
#include <protocol.h>
#include <stdarg.h>
#include <stdio.h>
#include <jsdos-log.h>

bool log_int21 = false;
bool log_fileio = false;
int log_dev_con = 0;

LOG_TYPES logType;
LOG_SEVERITIES logSeverity;

void jsdos_log(const char* tag, const char* message) {
  client_log(tag, message);
}
void jsdos_warn(const char* tag, const char* message) {
  client_warn(tag, message);
}
void jsdos_error(const char* tag, const char* message) {
  client_error(tag, message);
}

const char* LOG_TYPE_NAMES[] = {
    "LOG_ALL", "LOG_VGA",        "LOG_VGAGFX", "LOG_VGAMISC", "LOG_INT10",
    "LOG_SB",  "LOG_DMACONTROL", "LOG_FPU",    "LOG_CPU",     "LOG_PAGING",
    "LOG_FCB", "LOG_FILES",      "LOG_IOCTL",  "LOG_EXEC",    "LOG_DOSMISC",
    "LOG_PIT", "LOG_KEYBOARD",   "LOG_PIC",    "LOG_MOUSE",   "LOG_BIOS",
    "LOG_GUI", "LOG_MISC",       "LOG_IO",     "LOG_PCI",     "LOG_VOODOO",
    "LOG_NET", "LOG_MSG",        "LOG_MAX"};
constexpr int LOG_TYPE_NAMES_COUNT = sizeof(LOG_TYPE_NAMES) / sizeof(char*);

void Logger::operator()(char const* format, ...) {
  if (logSeverity != LOG_ERROR && !(logSeverity == LOG_NORMAL && logType == LOG_NET)) {
    return;
  }

  static char buf[1024];
  buf[1023] = 0;

  va_list msg;
  va_start(msg, format);
  vsprintf(buf, format, msg);
  va_end(msg);

  auto name = LOG_TYPE_NAMES[logType];
  switch (logSeverity) {
    case LOG_ERROR:
      jsdos_error(name, buf);
      break;
    case LOG_WARN:
      jsdos_warn(name, buf);
      break;
    default:
      jsdos_log(name, buf);
  }
}

Logger& getLogger(LOG_TYPES type, LOG_SEVERITIES severity) {
  static_assert(LOG_TYPE_NAMES_COUNT == LOG_MAX + 1);
  static Logger logger;
  logSeverity = severity;
  logType = type;
  return logger;
};
