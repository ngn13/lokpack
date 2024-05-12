#pragma once
#include <openssl/crypto.h>
#include <stdbool.h>
#include <stdlib.h>

#define OUTPUT_SIZE 1024
#define INPUT_SIZE  1012

EVP_PKEY_CTX *rsa_encrypt_init();
bool          rsa_encrypt(EVP_PKEY_CTX *, unsigned char *, size_t, unsigned char *, size_t *);
void          rsa_encrypt_free(EVP_PKEY_CTX *);

EVP_PKEY_CTX *rsa_decrypt_init();
bool          rsa_decrypt(EVP_PKEY_CTX *, unsigned char *, size_t, unsigned char *, size_t *);
void          rsa_decrypt_free(EVP_PKEY_CTX *);

bool rsa_init(char *, bool);
void rsa_free();
