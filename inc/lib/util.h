#pragma once
#include <sys/stat.h>
#include <stdbool.h>

#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

/* TODO: maybe add something like lp_join() to join paths */

#define lp_str_error() strerror(errno)
int lp_openssl_error(void);

#define lp_streq(s1, s2) (strcmp(s1, s2) == 0)
bool lp_startswith(char *str, char *pre, uint32_t pre_len);

bool lp_is_root(char *path);
int  lp_is_dir(char *path);
bool lp_has_ext(char *name, char *ext);
bool lp_has_exts(char *name, char **exts);

#define LP_SHA256_SIZE (EVP_MAX_MD_SIZE * 2 + 1)
char *lp_sha256(char *str, char *hash);

char **lp_split(char *str, char sep);
void   lp_split_free(char **list);
void   lp_replace(char *str, char old, char new);
