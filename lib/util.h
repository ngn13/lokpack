#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>

typedef struct clist {
  char **c;
  size_t s;
} clist_t;

bool eq(char *, char *);
int  join(char *, const char *, const char *);
bool startswith(char *, char *);
bool endswith(char *, char *);
bool has_valid_ext(char *, char *);
bool has_valid_exts(char *, clist_t *);
bool is_root_path(char *);

clist_t *clist_new();
clist_t *clist_from_str(char *);
void     clist_add(clist_t *, char *);
void     clist_free(clist_t *);

void  replace(char *, char, char);
char *get_md5(char *);
bool  copy_stat(int, struct stat *);
