#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include "lib/config.h"
#include "lib/util.h"
#include "lib/log.h"
#include "lib/rsa.h"

EVP_PKEY *lp_rsa_key_load(void) {
  EVP_PKEY *key = NULL;
#ifdef LP_PRIVKEY
  BIO *bio = BIO_new_mem_buf(LP_PRIVKEY, -1);
#else
  BIO *bio = BIO_new_mem_buf(LP_PUBKEY, -1);
#endif

  if (NULL == bio) {
    lp_debug("Failed to load key with BIO");
    lp_openssl_error();
    return false;
  }

#ifdef LP_PRIVKEY
  key = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
#else
  key = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
#endif
  BIO_free(bio);

  if (NULL == key) {
    lp_debug("Failed to initialize key");
    lp_openssl_error();
    return false;
  }

  return key;
}

void lp_rsa_key_free(EVP_PKEY *key) {
  if (NULL != key)
    EVP_PKEY_free(key);
}

void lp_rsa_init(lp_rsa_t *rsa, EVP_PKEY *key) {
  if (NULL != rsa && NULL != key) {
    memset(rsa, 0, sizeof(*rsa));
    rsa->key = key;
  }
}

bool lp_rsa_load(lp_rsa_t *rsa) {
  int      secret_len = LP_RSA_SECRET_SIZE;
  uint8_t *secrets[1] = {NULL};

  if (NULL == rsa) {
    errno = EINVAL;
    return false;
  }

  /* initialize the lp_rsa_t structure */
  if (NULL == (rsa->ctx = EVP_CIPHER_CTX_new())) {
    lp_debug("Failed to create a cipher ctx");
    lp_openssl_error();
    return false;
  }

  secrets[0] = rsa->secret;

#ifdef LP_PRIVKEY
  /*

   * BUG: for some reason OpenInit takes a lot of time on weaker hardware, this
   *      might be related to the key size or maybe something is wrong with my
   *      implementation here, idk

  */
  if (EVP_OpenInit(rsa->ctx,
          EVP_aes_256_cbc(),
          secrets[0],
          secret_len,
          rsa->iv,
          rsa->key) == 0) {
    lp_debug("Failed to initialize cipher ctx");
    lp_openssl_error();
    return false;
  }
#else
  if (EVP_SealInit(rsa->ctx,
          EVP_aes_256_cbc(),
          secrets,
          &secret_len,
          rsa->iv,
          &rsa->key,
          1) != 1) {
    lp_debug("Failed to initialize cipher ctx");
    lp_openssl_error();
    return false;
  }
#endif

  return true;
}

void lp_rsa_free(lp_rsa_t *rsa) {
  if (NULL == rsa)
    return;

  if (NULL != rsa->ctx)
    EVP_CIPHER_CTX_free(rsa->ctx);

  memset(rsa, 0, sizeof(*rsa));
}

bool lp_rsa_encrypt(
    lp_rsa_t *rsa, uint8_t *in, int in_len, uint8_t *out, int *out_len) {
#ifndef LP_PRIVKEY
  /* check the arguments */
  if (NULL == rsa || NULL == in || NULL == out || in_len <= 0) {
    errno = EINVAL;
    return false;
  }

  /* "seal" (encrypt) the provided "envelope" (data) */
  if (!EVP_SealUpdate(rsa->ctx, out, out_len, in, in_len)) {
    lp_debug("Failed to encrypt data");
    lp_openssl_error();
    return false;
  }

  return true;
#else
  (void)rsa;
  (void)in;
  (void)in_len;
  (void)out;
  (void)out_len;

  errno = ENOSYS;
  return false;
#endif
}

bool lp_rsa_decrypt(
    lp_rsa_t *rsa, uint8_t *in, int in_len, uint8_t *out, int *out_len) {
#ifdef LP_PRIVKEY
  /* check the arguments */
  if (NULL == rsa || NULL == in || NULL == out || NULL == out_len) {
    errno = EINVAL;
    return false;
  }

  /* "open" (decrypt) the provided "envelope" (data) */
  if (!EVP_OpenUpdate(rsa->ctx, out, out_len, in, in_len)) {
    lp_debug("Failed to decrypt data");
    lp_openssl_error();
    return false;
  }

  return true;
#else
  (void)rsa;
  (void)in;
  (void)in_len;
  (void)out;
  (void)out_len;

  errno = ENOSYS;
  return false;
#endif
}

bool lp_rsa_done(lp_rsa_t *rsa, uint8_t *out, int *out_len) {
  if (NULL == rsa || NULL == out || NULL == out_len) {
    errno = EINVAL;
    return false;
  }

#ifdef LP_PRIVKEY
  if (!EVP_OpenFinal(rsa->ctx, out, out_len)) {
    lp_debug("Failed to decrypt the final data");
    lp_openssl_error();
    return false;
  }
#else
  if (!EVP_SealFinal(rsa->ctx, out, out_len)) {
    lp_debug("Failed to encrypt the final data");
    lp_openssl_error();
    return false;
  }
#endif

  return true;
}
