#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <stdbool.h>

int decrypt(unsigned char *in, int sz, unsigned char *out) {
  int len = 0, ret = -1;
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  EVP_DecryptInit(ctx, EVP_aes_256_gcm(), BUILD_KEY, BUILD_IV);
  EVP_CIPHER_CTX_set_key_length(ctx, 256);

  EVP_DecryptUpdate(ctx, out, &len, in, sz);
  ret = len;

  EVP_DecryptFinal(ctx, out + len, &len);
  ret += len;

  EVP_CIPHER_CTX_free(ctx);
  return ret;
}

int encrypt(unsigned char *in, int sz, unsigned char *out) {
  int len = 0, ret = -1;
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  EVP_EncryptInit(ctx, EVP_aes_256_gcm(), BUILD_KEY, BUILD_IV);
  EVP_CIPHER_CTX_set_key_length(ctx, 256);

  EVP_EncryptUpdate(ctx, out, &len, in, sz);
  ret = len;

  EVP_EncryptFinal(ctx, out + len, &len);
  ret += len;

  EVP_CIPHER_CTX_free(ctx);
  return ret;
}
