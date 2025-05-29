#include <stdarg.h>
#include <stdio.h>

#include "lib/log.h"

/* epic macro for creating the logging functions */
#define lp_log_make(name, out, color, prefix)                                  \
  int lp_##name(const char *fmt, ...) {                                        \
    va_list args;                                                              \
    int     ret = 0;                                                           \
    va_start(args, fmt);                                                       \
    ret += fprintf(out, FG_BOLD color prefix FG_RESET " ");                    \
    ret += vfprintf(out, fmt, args);                                           \
    ret += fprintf(out, "\n");                                                 \
    va_end(args);                                                              \
    return ret;                                                                \
  }

/* clang-format off */

lp_log_make(info,    stdout, FG_BLUE,  "[*]")
lp_log_make(success, stdout, FG_GREEN, "[+]")
lp_log_make(fail,    stderr, FG_RED,   "[-]")

#if LP_DEBUG
lp_log_make(debug, stdout, FG_GRAY, "[*]")
#else
int lp_debug(const char *fmt, ...) {
  (void)fmt;
  return -1;
}
#endif
