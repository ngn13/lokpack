#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <stdbool.h>
#include <stdlib.h>

#include "log.h"
#include "rsa.h"

EVP_PKEY_CTX *ctx = NULL;
EVP_PKEY     *key = NULL;

void rsa_openssl_error(char *buf) {
  long err = ERR_get_error();
  ERR_error_string(err, buf);
}

bool rsa_init(char *buf, bool is_public) {
  BIO *bio = BIO_new_mem_buf(buf, -1);

  if (NULL == bio) {
    debug("Failed to load key BIO");
    return false;
  }

  if (is_public)
    key = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
  else
    key = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
  BIO_free(bio);

  if (NULL == key) {
    debug("Failed to read the key");
    return false;
  }

  if (!is_public) {
    ctx = EVP_PKEY_CTX_new(key, NULL);
    if (NULL == ctx) {
      debug("Failed to init ctx");
      return false;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
      debug("Failed to create a init decrypt ctx");
      return false;
    }
  }

  return true;
}

void rsa_free() {
  if (NULL != ctx)
    EVP_PKEY_CTX_free(ctx);

  if (NULL != key)
    EVP_PKEY_free(key);
}

bool rsa_decrypt(unsigned char *enc, size_t len, unsigned char *out, size_t *outlen) {
  if (EVP_PKEY_decrypt(ctx, out, outlen, enc, len) <= 0) {
    char err[256];
    rsa_openssl_error(err);

    debug("Failed to decrypt data: %s", err);
    return false;
  }
  return true;
}

EVP_PKEY_CTX *rsa_encrypt_init() {
  EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(key, NULL);
  if (NULL == ctx)
    return NULL;

  EVP_PKEY_encrypt_init(ctx);
  return ctx;
}

bool rsa_encrypt(EVP_PKEY_CTX *c, unsigned char *plain, size_t len, unsigned char *out, size_t *outlen) {
  if (EVP_PKEY_encrypt(c, out, outlen, plain, len) <= 0) {
    char err[256];
    rsa_openssl_error(err);

    debug("Failed to decrypt data: %s", err);
    return false;
  }
  return true;
}

void rsa_encrypt_free(EVP_PKEY_CTX *c) {
  if (NULL != c)
    EVP_PKEY_CTX_free(c);
}
