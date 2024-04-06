#include "util.h"
#include <ctype.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_md5(char *s) {
  unsigned char digest[32];
  unsigned int digest_len;

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  const EVP_MD *alg = EVP_sha256();
  EVP_DigestInit_ex(ctx, alg, NULL);
  EVP_DigestUpdate(ctx, s, strlen(s));
  EVP_DigestFinal(ctx, digest, &digest_len);
  EVP_MD_CTX_free(ctx);

  char *ret = malloc((sizeof(digest) * 2) + 1);
  for (int i = 0; i < sizeof(digest); ++i)
    sprintf(&ret[i * 2], "%02x", (unsigned int)digest[i]);
  return ret;
}

void replace(char *s, char o, char n) {
  int sz = strlen(s);
  for (int i = 0; i < sz; i++) {
    if (s[i] == o)
      s[i] = n;
  }
}

bool has_valid_ext(char *file, clist_t *list) {
  for (int i = 0; i < list->s; i++) {
    if(eq(list->c[i], "ALL"))
      return true;

    char ext[strlen(list->c[i]) + 2];
    sprintf(ext, ".%s", list->c[i]);

    if (endswith(file, ext))
      return true;
  }
  return false;
}

bool is_root_path(char *str) {
  int len = strlen(str), c = 0;
  for (int i = 0; i < len; i++) {
    if (str[i] == '/')
      c++;
    if (c > 1)
      return false;
  }
  return true;
}

bool startswith(char *str, char *sub) {
  if (strlen(sub) > strlen(str))
    return false;
  return strncmp(str, sub, strlen(sub)) == 0;
}

bool endswith(char *str, char *sub) {
  int strl = strlen(str);
  int subl = strlen(sub);

  if (subl > strl)
    return false;
  return strncmp(str + strl - subl, sub, subl) == 0;
}

int join(char *res, const char *base, const char *pth) {
  int blen = strlen(base);

  if ((base[blen - 1] == '/' && pth[0] != '/') ||
      (base[blen - 1] != '/' && pth[0] == '/')) {
    return sprintf(res, "%s%s", base, pth);
  } else if (base[blen - 1] != '/' && pth[0] != '/') {
    return sprintf(res, "%s/%s", base, pth);
  } else if (base[blen - 1] == '/' && pth[0] == '/') {
    char *basedup = strdup(base);
    basedup[blen - 1] = '\0';

    return sprintf(res, "%s%s", basedup, pth);
  }

  return -1;
}

clist_t *clist_new() {
  clist_t *l = malloc(sizeof(clist_t));
  l->c = NULL;
  l->s = 0;
  return l;
}

clist_t *clist_from_str(char *str) {
  char *save = NULL, *el = NULL;
  char *strdp = strdup(str);
  clist_t *l = clist_new();
  el = strtok_r(strdp, ",", &save);

  while (NULL != el) {
    clist_add(l, strdup(el));
    el = strtok_r(NULL, ",", &save);
  }

  free(strdp);
  return l;
}

void clist_free(clist_t *l) {
  if (NULL == l->c || 0 == l->s)
    return;

  for (int i = 0; i < l->s; i++)
    free(l->c[i]);
  free(l->c);
  l->s = 0;
  return;
}

void clist_add(clist_t *l, char *en) {
  if (NULL == l->c || 0 == l->s) {
    l->c = malloc(sizeof(char *));
    l->c[l->s] = en;
    l->s++;
    return;
  }

  l->c = realloc(l->c, sizeof(char *) * (l->s + 1));
  l->c[l->s] = en;
  l->s++;
  return;
}

bool eq(char *s1, char *s2) {
  if (strlen(s1) != strlen(s2))
    return false;
  return strcmp(s1, s2) == 0;
}
