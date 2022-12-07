//
// Created by Alexander Guryanov on 07/12/22.
//

#ifndef JS_DOS_JSDOS_LOG_H
#define JS_DOS_JSDOS_LOG_H

enum LOG_TYPES {
  LOG_ALL, LOG_VGA, LOG_VGAGFX,LOG_VGAMISC,LOG_INT10,
  LOG_SB,LOG_DMACONTROL,LOG_FPU,LOG_CPU,LOG_PAGING,
  LOG_FCB,LOG_FILES,LOG_IOCTL,LOG_EXEC,LOG_DOSMISC,
  LOG_PIT,LOG_KEYBOARD,LOG_PIC,LOG_MOUSE,LOG_BIOS,
  LOG_GUI,LOG_MISC,LOG_IO,LOG_PCI,LOG_VOODOO,
  LOG_NET,LOG_MSG,LOG_MAX
};

enum LOG_SEVERITIES {
  LOG_DEBUG,
  LOG_NORMAL,
  LOG_WARN,
  LOG_ERROR,
  LOG_FATAL,
  LOG_NEVER,
  LOG_SEVERITY_MAX
};

struct Logger {
  void operator() (char const* format, ...);
};

Logger& getLogger(LOG_TYPES type, LOG_SEVERITIES severity);

#define LOG(type,severity) getLogger(type,severity)
#define UNBLOCKED_LOG(type,severity) getLogger(type,severity)
#define LOG_MSG(format,...)  getLogger(LOG_MSG, LOG_NORMAL)(format, ##__VA_ARGS__)
#define LOG_ERR(format,...)  getLogger(LOG_MSG, LOG_ERROR)(format, ##__VA_ARGS__)
#define DEBUG_ShowMsg(format,...)  getLogger(LOG_MSG, LOG_NORMAL)(format, ##__VA_ARGS__)

#endif  // JS_DOS_JSDOS_LOG_H
