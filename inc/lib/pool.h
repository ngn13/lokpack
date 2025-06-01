#pragma once
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

/* pool work function */
typedef void (*lp_pool_func_t)(void *);

typedef struct {
  bool     running;      /* is the pool running */
  uint32_t thread_count; /* total thread count for the pool */

  pthread_mutex_t lock;      /* locked before modifying the pool structure */
  pthread_cond_t  work_cond; /* used after updating the work queue */
  pthread_cond_t  quit_cond; /* used for waiting threads to quit */

  uint32_t completed; /* completed work count */
  uint32_t total;     /* total work count */
  uint32_t len;       /* current work queue length */
  struct lp_work {
    lp_pool_func_t  func;
    void           *data;
    struct lp_work *next;
  } *queue; /* work queue */
} lp_pool_t;

lp_pool_t *lp_pool_init(lp_pool_t *pool, uint32_t size);
void       lp_pool_free(lp_pool_t *pool);
void       lp_pool_reset(lp_pool_t *pool);
bool       lp_pool_add(lp_pool_t *pool, lp_pool_func_t func, void *data);
void       lp_pool_wait(lp_pool_t *pool, bool bar);
