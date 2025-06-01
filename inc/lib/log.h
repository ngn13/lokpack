#pragma once
#include <stdint.h>

#define LP_LOG_RED   "\x1b[31m"
#define LP_LOG_BOLD  "\x1b[1m"
#define LP_LOG_BLUE  "\x1b[34m"
#define LP_LOG_GREEN "\x1b[32m"
#define LP_LOG_GRAY  "\x1b[37m"
#define LP_LOG_RESET "\x1b[0m"
#define LP_LOG_CLEAR "\x1b[2K"

int lp_info(const char *, ...);
int lp_success(const char *, ...);
int lp_fail(const char *, ...);
int lp_debug(const char *, ...);
int lp_bar(uint32_t max, uint32_t pos);
