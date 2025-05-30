#pragma once
#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <stdbool.h>
#include <stdint.h>

#define LP_RSA_BLOCK_SIZE  (4096)
#define LP_RSA_SECRET_SIZE (1024)
#define LP_RSA_IV_SIZE     (16)

typedef struct {
  EVP_CIPHER_CTX *ctx;
  uint8_t         secret[LP_RSA_SECRET_SIZE], iv[LP_RSA_IV_SIZE];
} lp_rsa_t;

bool lp_rsa_key_load(void);
void lp_rsa_key_free(void);

void lp_rsa_init(lp_rsa_t *rsa);

bool lp_rsa_load(lp_rsa_t *rsa);
void lp_rsa_free(lp_rsa_t *rsa);

bool lp_rsa_encrypt(lp_rsa_t *rsa, uint8_t *buf, int in_len, int *out_len);
bool lp_rsa_decrypt(lp_rsa_t *rsa, uint8_t *buf, int in_len, int *out_len);
bool lp_rsa_done(lp_rsa_t *rsa, uint8_t *buf, int *len);
