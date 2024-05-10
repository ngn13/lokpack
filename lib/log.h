#pragma once
#include <stdbool.h>
#define FG_RED   "\x1b[31m"
#define FG_BOLD  "\x1b[1m"
#define FG_BLUE  "\x1b[34m"
#define FG_GREEN "\x1b[32m"
#define FG_GRAY  "\x1b[37m"
#define FG_RESET "\x1b[0m"

extern bool DEBUG;

void log_init();
void info(const char *, ...);
void error(const char *, ...);
void success(const char *, ...);
void debug(const char *, ...);
