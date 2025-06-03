#include "lib/traverse.h"
#include "lib/pool.h"
#include "lib/util.h"
#include "lib/log.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

/*

 * traverser traverses a directory/path and looks for files with specific file
 * extensions, using the lp_pool_t thread pool (see lib/pool.h) - when a valid
 * file is found, a configurable handler is called

*/

#define TRAV_DEF_MODE (R_OK)

static pthread_mutex_t lp_trav_lock;            /* thread lock for traverser */
static bool            lp_trav_running = false; /* is the traverser running? */
static lp_pool_t      *lp_trav_pool    = NULL;  /* traverser thread pool */
static char          **lp_trav_target  = NULL;  /* target extensions */
static char          **lp_trav_ignore  = NULL;  /* extensions to ignore */
static int             lp_trav_mode    = TRAV_DEF_MODE; /* target file mode */
static lp_handler_t    lp_trav_handler = NULL;          /* handler function */

#define trav_lock()   pthread_mutex_lock(&lp_trav_lock)
#define trav_unlock() pthread_mutex_unlock(&lp_trav_lock)

bool lp_traverser_init(uint32_t threads, char **target, char **ignore) {
  /* thread pool & lock can only be initialized once */
  if (NULL == lp_trav_pool && threads > 0) {
    lp_trav_pool = lp_pool_init(NULL, threads);
    pthread_mutex_init(&lp_trav_lock, NULL);
  }

  lp_trav_target = target;
  lp_trav_ignore = ignore;

  return NULL != lp_trav_pool;
}

void lp_traverser_free(void) {
  if (NULL == lp_trav_pool)
    return;

  lp_pool_free(lp_trav_pool);
  pthread_mutex_destroy(&lp_trav_lock);

  lp_trav_running = false;
  lp_trav_target  = NULL;
  lp_trav_ignore  = NULL;
  lp_trav_mode    = TRAV_DEF_MODE;
  lp_trav_handler = NULL;
}

void lp_traverser_set_mode(int mode) {
  lp_pool_wait(lp_trav_pool, false);
  lp_trav_mode = mode;
}

void lp_traverser_set_handler(lp_handler_t handler) {
  lp_pool_wait(lp_trav_pool, false);
  lp_trav_handler = handler;
}

#define trav_pool_add(func, data) lp_pool_add(lp_trav_pool, func, data)
#define trav_file(path)           trav_pool_add(_trav_file, path)
#define trav_dir(path)            trav_pool_add(_trav_dir, path)

bool _trav_should_stop(void) {
  bool running = false;

  /* check if the traverser should be running */
  trav_lock();
  running = lp_trav_running;
  trav_unlock();

  return !running;
}

void _trav_file(void *file_path) {
  if (!_trav_should_stop() && NULL != lp_trav_handler)
    lp_trav_handler((char *)file_path);
  free(file_path);
}

void _trav_dir(void *_dir_path) {
  struct dirent *entry;
  char          *dir_path = _dir_path;
  DIR           *dir_ptr  = opendir(dir_path);
  int            dir_fd   = -1;

  char *path     = NULL;
  int   path_len = 0;

  if (NULL == dir_ptr)
    goto end;

  if ((dir_fd = dirfd(dir_ptr)) < 0)
    goto end;

  while (!_trav_should_stop() && NULL != (entry = readdir(dir_ptr))) {
    /* ignore current and previous directory */
    if (lp_streq(entry->d_name, ".") || lp_streq(entry->d_name, ".."))
      continue;

    /* we are only looking for files and directories */
    if (DT_REG != entry->d_type && DT_DIR != entry->d_type)
      continue;

    /* check for the specified access mode */
    if (faccessat(dir_fd, entry->d_name, lp_trav_mode, 0) != 0)
      continue;

    /* check for target and ignored file extensions */
    if (entry->d_type == DT_REG) {
      if (NULL != lp_trav_target && !lp_has_exts(entry->d_name, lp_trav_target))
        continue;

      if (NULL != lp_trav_ignore && lp_has_exts(entry->d_name, lp_trav_ignore))
        continue;
    }

    /* get the full target path */
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

    /*

     * new "path" pointer will used the other traverser function or it will
     * passed to the handler (see lp_traverser_set_handler()), so these
     * functions are responsible for freeing it when they are done with it

    */
    switch (entry->d_type) {
    case DT_REG:
      lp_debug("Traversed file: %s", path);
      trav_file(path);
      break;

    case DT_DIR:
      lp_debug("Traversed directory: %s", path);
      trav_dir(path);
      break;

    default: /* not possible to reach here - just to shut up clang-tidy */
      free(path);
    }

    /* just to prevent any double-free or UAFs */
    path = NULL;
  }

end:
  if (NULL != dir_ptr)
    closedir(dir_ptr);
  free(dir_path);
}

bool lp_traverser_run(char *path) {
  if (NULL == path) {
    errno = EINVAL;
    return false;
  }

  /* to run the traverser, it should be initialized with lp_traverser_init() */
  if (NULL == lp_trav_pool) {
    errno = EAGAIN;
    return false;
  }

  if (NULL == (path = strdup(path)))
    return false; /* errno set by strdup() */

  lp_trav_running = true;

  switch (lp_is_dir(path)) {
  case 0: /* directory */
    trav_dir(path);
    break;

  case 1: /* file */
    trav_file(path);
    break;

  default: /* error */
    free(path);
    return false;
  }

  return true;
}

void lp_traverser_stop(void) {
  if (NULL == lp_trav_pool)
    return;

  trav_lock();
  lp_trav_running = false;
  trav_unlock();
}

void lp_traverser_wait(bool bar) {
  if (NULL == lp_trav_pool)
    return;

  /* wait for all the pool threads and reset the work stats */
  lp_pool_wait(lp_trav_pool, bar);
  lp_pool_reset(lp_trav_pool);
}
