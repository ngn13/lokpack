#include <sys/ioctl.h>
#include <stdarg.h>
#include <unistd.h>

#include <stdio.h>
#include <errno.h>

#include "lib/log.h"

int lp_bar(uint32_t max, uint32_t pos) {
  /* percentage stuff */
  char  perc_buf[5];
  int   perc, perc_len;
  float cur = (float)pos / (float)max;

  /* used for calculating bar length and stuff */
  struct winsize window;
  int            size = 0, total, prog, i;

  /* calculate the completion percentage */
  if ((perc = 100 * cur) > 100) {
    errno = EINVAL;
    return -1;
  }

  /* get the window size for stdout */
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) < 0)
    return -1;

  /* convert percentage to string and calculate column size */
  if ((perc_len = snprintf(perc_buf, sizeof(perc_buf), "%%%3u", perc)) !=
      (int)sizeof(perc_buf) - 1)
    return -1;

  if ((total = window.ws_col - perc_len - 3) <= 0) {
    errno = EINVAL;
    return -1;
  }

  /* draw the bar */
  prog = total * cur;
  size += printf("\r" LP_LOG_BOLD "%s " LP_LOG_BLUE "[", perc_buf);

  for (i = 0; i < prog; i++)
    size += printf(LP_LOG_BLUE "=");

  size += printf(LP_LOG_RESET);

  for (i = prog; i < total; i++)
    size += printf(LP_LOG_BLUE "-");

  size += printf(LP_LOG_BOLD "]" LP_LOG_RESET);
  fflush(stdout);
  return size;
}

/* epic macro for creating the logging functions */
#define lp_log_make(name, out, color, prefix)                                  \
  int lp_##name(const char *fmt, ...) {                                        \
    va_list args;                                                              \
    int     ret = 0;                                                           \
    va_start(args, fmt);                                                       \
    ret += fprintf(                                                            \
        out, "\r" LP_LOG_CLEAR LP_LOG_BOLD color prefix LP_LOG_RESET " ");     \
    ret += vfprintf(out, fmt, args);                                           \
    ret += fprintf(out, "\n");                                                 \
    va_end(args);                                                              \
    return ret;                                                                \
  }

/* clang-format off */

lp_log_make(info,    stdout, LP_LOG_BLUE,  "[*]")
lp_log_make(success, stdout, LP_LOG_GREEN, "[+]")
lp_log_make(fail,    stderr, LP_LOG_RED,   "[-]")

#if LP_DEBUG
lp_log_make(debug, stdout, LP_LOG_GRAY, "[*]")
#else
int lp_debug(const char *fmt, ...) {
  (void)fmt;
  return -1;
}
#endif
