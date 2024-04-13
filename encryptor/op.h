#pragma once
#include <stdbool.h>

#include "../lib/log.h"

#define TYPE_BOOL 1
#define TYPE_STR 2
#define TYPE_INT 3

typedef struct Option {
  char *value;
  char *name;
  char *desc;
  int type;
} option_t;

extern option_t options[];
char *extract_value(char *);
bool parse_opt(char *);
bool get_bool(char *);
char *get_str(char *);
int get_int(char *);
void print_opts();
