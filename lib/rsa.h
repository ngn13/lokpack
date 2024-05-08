#pragma once
#include <openssl/crypto.h>
#include <stdbool.h>
#include <stdlib.h>

#define OUTPUT_SIZE 512
#define INPUT_SIZE  500

EVP_PKEY_CTX *rsa_encrypt_init();
bool          rsa_decrypt(unsigned char *, size_t, unsigned char *, size_t *);
bool          rsa_encrypt(EVP_PKEY_CTX *, unsigned char *, size_t, unsigned char *, size_t *);
void          rsa_encrypt_free(EVP_PKEY_CTX *);

bool rsa_init(char *, bool);
void rsa_free();