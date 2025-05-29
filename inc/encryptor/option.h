#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  OPT_TYPE_BOOL,
  OPT_TYPE_LIST,
  OPT_TYPE_STR,
  OPT_TYPE_INT
} option_type_t;

typedef struct {
  /* option name, description and type */
  char         *name;
  char         *desc;
  option_type_t type;

  /* fields for different types of values */
  bool   value_bool;
  char **value_list;
  char  *value_str;
  int    value_int;
} option_t;

#define opt_empty(opt) (NULL == opt || *opt == 0)

bool opt_parse(char *option);
void opt_print(void);
void opt_help(void);

bool   opt_bool(char *name);
char **opt_list(char *name);
char  *opt_str(char *name);
int    opt_int(char *name);
