#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/log.h"
#include "../lib/util.h"
#include "op.h"

option_t options[] = {
    {.name = "threads",
     .type = TYPE_INT,
     .value = "20",
     .desc = "Thread count for the thread pool"},
    {.name = "paths",
     .type = TYPE_STR,
     .value = "/example",
     .desc = "Paths to look for files"},
    {.name = "exts",
     .type = TYPE_STR,
     .value = "txt,pdf,db,sql",
     .desc = "Valid extensions for files"},
    {.name = "ftp-url",
     .type = TYPE_STR,
     .value = "ftp://example",
     .desc = "Address for the FTP(S) server"},
    {.name = "ftp-user",
     .type = TYPE_STR,
     .value = "anonymous",
     .desc = "FTP(S) username"},
    {.name = "ftp-pwd",
     .type = TYPE_STR,
     .value = "anonymous",
     .desc = "FTP(S) password"},
    {.name = "no-ftp",
     .type = TYPE_BOOL,
     .value = "false",
     .desc = "Disable stealing files with FTP(S)"},
    {.name = "destruct",
     .type = TYPE_BOOL,
     .value = "false",
     .desc = "Self destruct the program"},
    {.name = "debug",
     .type = TYPE_BOOL,
     .value = "false",
     .desc = "Enable debug output"},
};

char *extract_value(char *o) {
  char *value = strtok(o, "=");
  if (NULL == value)
    return NULL;

  value = strtok(NULL, "=");
  if (NULL == value)
    return NULL;

  return value;
}

void print_opts() {
  int max_len = 0;
  for (int i = 0; i < sizeof(options) / sizeof(option_t); i++) {
    if (max_len < strlen(options[i].name))
      max_len = strlen(options[i].name);
  }

  for (int i = 0; i < sizeof(options) / sizeof(option_t); i++) {
    char spacebuf[(max_len - strlen(options[i].name)) + 2];
    for (int i = 0; i < sizeof(spacebuf); i++)
      spacebuf[i] = ' ';
    spacebuf[sizeof(spacebuf) - 1] = '\0';

    if (options[i].type == TYPE_BOOL)
      printf(FG_BOLD "    %s" FG_RESET "%s=> %s\n" FG_RESET, options[i].name,
             spacebuf,
             get_bool(options[i].name) ? FG_GREEN "true" : FG_RED "false");
    else
      printf(FG_BOLD "    %s" FG_RESET "%s=> %s\n" FG_RESET, options[i].name,
             spacebuf, options[i].value);
  }
}

int get_int(char *name) {
  for (int i = 0; i < sizeof(options) / sizeof(option_t); i++) {
    if (eq(options[i].name, name))
      return atoi(options[i].value);
  }

  return -1;
}

bool get_bool(char *name) {
  for (int i = 0; i < sizeof(options) / sizeof(option_t); i++) {
    if (eq(options[i].name, name))
      return eq(options[i].value, "true");
  }

  return false;
}

char *get_str(char *name) {
  for (int i = 0; i < sizeof(options) / sizeof(option_t); i++) {
    if (eq(options[i].name, name))
      return options[i].value;
  }

  return NULL;
}

void print_help() {
  info("Listing available options:");

  int max_len = 0;
  for (int i = 0; i < sizeof(options) / sizeof(option_t); i++) {
    if (max_len < strlen(options[i].name))
      max_len = strlen(options[i].name);
  }

  for (int i = 0; i < sizeof(options) / sizeof(option_t); i++) {
    char spacebuf[(max_len - strlen(options[i].name)) + 2];
    for (int i = 0; i < sizeof(spacebuf); i++)
      spacebuf[i] = ' ';
    spacebuf[sizeof(spacebuf) - 1] = '\0';

    printf(FG_BOLD "    --%s" FG_RESET "%s=> %s\n" FG_RESET, options[i].name,
           spacebuf, options[i].desc);
  }

  printf("\n");
}

bool parse_opt(char *o) {
  if (eq(o, "--help") || eq(o, "-h")) {
    print_help();
    exit(EXIT_SUCCESS);
  }

  for (int i = 0; i < sizeof(options) / sizeof(option_t); i++) {
    char fullop[strlen(options[i].name) + 5];
    sprintf(fullop, "--%s", options[i].name);
    if (!startswith(o, fullop))
      continue;

    switch (options[i].type) {
    case TYPE_BOOL:
      if (!eq(o, fullop))
        goto UNKNOWN;
      options[i].value = "true";
      return true;
      break;
    case TYPE_STR:
      options[i].value = extract_value(o);
      if (NULL == options[i].value) {
        error("Value not specified for the option: %s", options[i].name);
        return false;
      }
      return true;
      break;
    case TYPE_INT:
      options[i].value = extract_value(o);
      if (NULL == options[i].value) {
        error("Value not specified for the option: %s", options[i].name);
        return false;
      }

      if (atoi(options[i].value) <= 0) {
        error("Bad value for option: %s", options[i].name);
        return false;
      }

      return true;
      break;
    }
  }

UNKNOWN:
  error("Unknown option: %s", o);
  return false;
}
