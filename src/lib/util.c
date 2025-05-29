#include <openssl/err.h>
#include <openssl/evp.h>

#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "lib/util.h"
#include "lib/log.h"

bool lp_startswith(char *str, char *pre, uint32_t pre_len) {
  if (NULL == str || NULL == pre) {
    errno = EINVAL;
    return false;
  }

  if (pre_len == 0)
    pre_len = strlen(pre);

  if (pre_len > strlen(str))
    return false;

  return strncmp(str, pre, pre_len) == 0;
}

bool lp_is_root(char *str) {
  if (*str != '/')
    return false;

  for (str++; *str != 0; str++)
    if (*str == '/')
      return false;

  return true;
}

int lp_is_dir(char *path) {
  struct stat st;

  if (NULL == path) {
    errno = EINVAL;
    return -1;
  }

  if (stat(path, &st) < 0)
    return -1;

  return st.st_mode & S_IFDIR ? 0 : 1;
}

bool lp_has_ext(char *name, char *ext) {
  uint32_t ext_len = strlen(ext), name_len = strlen(name);

  if (ext_len >= name_len)
    return false;

  for (; *name != 0; name++)
    ;

  if (*(name -= ext_len + 1) != '.')
    return false;

  return lp_streq(++name, ext);
}

bool lp_has_exts(char *name, char **exts) {
  char **pos = exts;

  for (; NULL != *pos; pos++)
    if (lp_has_ext(name, *pos))
      return true;

  return false;
}

int lp_openssl_error(void) {
#if LP_DEBUG
  int  err = 0, count = 0;
  char buf[256];

  for (; (err = ERR_get_error()) != 0;) {
    ERR_error_string_n(err, buf, sizeof(buf));
    lp_debug("openssl error %d: %s", ++count, buf);
  }

  return count;
#else
  return -1;
#endif
}

char *lp_sha256(char *str, char *hash) {
  uint32_t i;

  /* stores the final hash */
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int  digest_len;

  /* calculate the hash with OpenSSL */
  EVP_MD_CTX   *ctx = EVP_MD_CTX_new();
  const EVP_MD *alg = EVP_sha256();

  if (NULL == ctx || NULL == alg) {
    lp_debug("Failed to create ctx or SHA256 alg");
    lp_openssl_error();
    EVP_MD_CTX_free(ctx);
    return NULL;
  }

  if (!EVP_DigestInit_ex(ctx, alg, NULL)) {
    lp_debug("Failed to initialize the hash digest");
    lp_openssl_error();
    EVP_MD_CTX_free(ctx);
    return NULL;
  }

  if (!EVP_DigestUpdate(ctx, str, strlen(str))) {
    lp_debug("Failed to update the hash digest");
    lp_openssl_error();
    EVP_MD_CTX_free(ctx);
    return NULL;
  }

  if (!EVP_DigestFinal(ctx, digest, &digest_len)) {
    lp_debug("Failed to finalize the digest");
    lp_openssl_error();
    EVP_MD_CTX_free(ctx);
    return NULL;
  }

  EVP_MD_CTX_free(ctx);

  /* allocate final buffer which will store the hex encoded hash */
  if (NULL == hash)
    hash = calloc(1, LP_SHA256_SIZE);
  else
    memset(hash, 0, LP_SHA256_SIZE);

  for (i = 0; i < sizeof(digest); ++i)
    sprintf(&hash[i * 2], "%02x", digest[i]);

  return hash;
}

char **lp_split(char *str, char sep) {
  char   **list = NULL, **pos = NULL, *cur = NULL;
  uint32_t count = 0;

  /* count how many elements are there */
  for (cur = str; *cur != 0; cur++)
    if (*cur == sep)
      count++;

  /* allocate the list */
  if (NULL == (pos = list = calloc(1, sizeof(*list) * (count + 1))))
    return NULL;

  *pos = str;

  /* split all the elements */
  for (cur = str; *cur != 0; cur++) {
    if (*cur != sep)
      continue;

    *cur     = 0;
    *(++pos) = cur + 1;
  }

  return list;
}

void lp_replace(char *str, char old, char new) {
  for (; *str != 0; str++)
    if (*str == old)
      *str = new;
}
