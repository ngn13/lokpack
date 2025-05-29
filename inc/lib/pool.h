#pragma once
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

typedef void (*lp_pool_func_t)(void *);

typedef struct {
  bool running;

  uint32_t thread_count;
  uint32_t queue_len;

  pthread_mutex_t lock;
  pthread_cond_t  work_cond;
  pthread_cond_t  quit_cond;

  struct lp_work {
    lp_pool_func_t  func;
    void           *data;
    struct lp_work *next;
  } *queue;
} lp_pool_t;

lp_pool_t *lp_pool_init(lp_pool_t *pool, uint32_t size);
bool       lp_pool_add(lp_pool_t *pool, lp_pool_func_t func, void *data);
bool       lp_pool_wait(lp_pool_t *pool, bool bar);
void       lp_pool_free(lp_pool_t *pool);
