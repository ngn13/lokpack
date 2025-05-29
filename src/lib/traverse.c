#include "lib/traverse.h"
#include "lib/pool.h"
#include "lib/util.h"
#include "lib/log.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

bool lp_traverser_setup(lp_traverser_t *trav, uint32_t threads, char **exts) {
  if (NULL == trav) {
    errno = EINVAL;
    return false;
  }

  trav->exts = exts;
  trav->mode = R_OK;

  return NULL != lp_pool_init(&trav->pool, threads);
}

void lp_traverser_free(lp_traverser_t *trav) {
  if (NULL == trav)
    return;

  lp_pool_free(&trav->pool);
  memset(trav, 0, sizeof(*trav));
}

void _traverser_file(lp_traverser_t *trav, char *file_path) {
  if (NULL != trav->handler)
    lp_pool_add(&trav->pool, (void (*)(void *))trav->handler, file_path);
  else
    free(file_path);
}

void _traverser_dir(lp_traverser_t *trav, char *dir_path) {
  struct dirent *entry;
  DIR           *dir_ptr = opendir(dir_path);
  int            dir_fd  = -1;

  char *path     = NULL;
  int   path_len = 0;

  if (NULL == dir_ptr)
    return;

  if ((dir_fd = dirfd(dir_ptr)) < 0) {
    closedir(dir_ptr);
    return;
  }

  while (NULL != (entry = readdir(dir_ptr))) {
    if (lp_streq(entry->d_name, ".") || lp_streq(entry->d_name, ".."))
      continue;

    if (DT_REG != entry->d_type && DT_DIR != entry->d_type)
      continue;

    if (faccessat(dir_fd, entry->d_name, trav->mode, 0) != 0)
      continue;

    if (entry->d_type == DT_REG && NULL != trav->exts &&
        !lp_has_exts(entry->d_name, trav->exts))
      continue;

    path_len = strlen(dir_path) + strlen(entry->d_name) + 1;

    if (NULL == (path = calloc(1, path_len + 1))) {
      lp_debug(
          "Failed to allocate buffer for path: %s/%s", dir_path, entry->d_name);
      continue;
    }

    if ((snprintf(path, path_len + 1, "%s/%s", dir_path, entry->d_name)) !=
        path_len) {
      lp_debug("Failed to format the buffer for path: %s/%s",
          dir_path,
          entry->d_name);
      free(path);
      continue;
    }

    switch (entry->d_type) {
    case DT_REG:
      lp_debug("Traversed file: %s", path);
      _traverser_file(trav, path);
      break;

    case DT_DIR:
      lp_debug("Traversing directory: %s", path);
      _traverser_dir(trav, path);
      break;

    default: /* not possible to reach here - just to shut up clang-tidy */
      free(path);
    }
  }

  closedir(dir_ptr);
  free(dir_path);
}

bool lp_traverser_run(lp_traverser_t *trav, char *path) {
  if (NULL == trav || NULL == path) {
    errno = EINVAL;
    return false;
  }

  if (NULL == (path = strdup(path)))
    return false; /* errno set by strdup() */

  switch (lp_is_dir(path)) {
  case 0: /* directory */
    _traverser_dir(trav, path);
    break;

  case 1: /* file */
    _traverser_file(trav, path);
    break;

  default: /* error */
    return false;
  }

  return true;
}
