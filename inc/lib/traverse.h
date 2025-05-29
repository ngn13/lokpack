#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "lib/pool.h"

/*

 * traverser traverses a directory/path and looks for files with specific file
 * extensions

 * when a valid file is found, a configurable handler is called

 * to make things faster each directory and file is traversed in a new thread,
 * see lib/traverse.c for more info

*/

typedef struct {
  /* traverser stuff */
  lp_pool_t pool;
  char    **exts;
  int       mode;
  void (*handler)(char *path);
} lp_traverser_t;

#define lp_traverser_init(trav) memset((trav), 0, sizeof(lp_traverser_t))

bool lp_traverser_setup(lp_traverser_t *trav, uint32_t threads, char **exts);
bool lp_traverser_run(lp_traverser_t *trav, char *path);
void lp_traverser_free(lp_traverser_t *trav);

#define lp_traverser_set_handler(trav, h) ((trav)->handler = h)
#define lp_traverser_set_mode(trav, m)    ((trav)->mode = m)
#define lp_traverser_wait(trav, display)  lp_pool_wait(&(trav)->pool, (display))
