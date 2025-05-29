#pragma once
#define FG_RED   "\x1b[31m"
#define FG_BOLD  "\x1b[1m"
#define FG_BLUE  "\x1b[34m"
#define FG_GREEN "\x1b[32m"
#define FG_GRAY  "\x1b[37m"
#define FG_RESET "\x1b[0m"

int lp_info(const char *, ...);
int lp_success(const char *, ...);
int lp_fail(const char *, ...);
int lp_debug(const char *, ...);
