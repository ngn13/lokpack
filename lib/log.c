#include "log.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

bool DEBUG = false;

#ifdef _WIN64

#include <windows.h>

void log_init() {
  DWORD  mode;
  HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

  if (GetConsoleMode(out, &mode) == 0)
    return;

  mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(out, mode);
}

#else

void log_init(){};

#endif

void info(const char *msg, ...) {
  va_list args;
  va_start(args, msg);

  printf(FG_BOLD FG_BLUE "[*] " FG_RESET);
  vprintf(msg, args);
  printf("\n");

  va_end(args);
}

void error(const char *msg, ...) {
  va_list args;
  va_start(args, msg);

  printf(FG_BOLD FG_RED "[-] " FG_RESET);
  vprintf(msg, args);
  printf("\n");

  va_end(args);
}

void success(const char *msg, ...) {
  va_list args;
  va_start(args, msg);

  printf(FG_BOLD FG_GREEN "[+] " FG_RESET);
  vprintf(msg, args);
  printf("\n");

  va_end(args);
}

void debug(const char *msg, ...) {
  if (!DEBUG)
    return;

  va_list args;
  va_start(args, msg);

  printf(FG_BOLD FG_GRAY "[*] " FG_RESET);
  vprintf(msg, args);
  printf("\n");

  va_end(args);
}
