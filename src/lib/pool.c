#include "lib/pool.h"

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <stdio.h>
#include <errno.h>

#define pool_lock()   pthread_mutex_lock(&pool->lock)
#define pool_unlock() pthread_mutex_unlock(&pool->lock)

void *_pool_worker(void *_pool) {
  lp_pool_t      *pool = _pool;
  struct lp_work *work = NULL;

  pool_lock();
  pool->thread_count++;
  pool_unlock();

  while (true) {
    pool_lock();

    /* wait for work */
    while (pool->running && NULL == pool->queue)
      pthread_cond_wait(&pool->work_cond, &pool->lock);

    if (!pool->running)
      break;

    /* get the next work */
    if (NULL != (work = pool->queue))
      pool->queue = pool->queue->next;

    pool_unlock();

    /* do the work (if any) */
    if (NULL == work)
      continue;

    work->func(work->data);
    free(work);
    work = NULL;

    /* notify lp_pool_wait() */
    pool_lock();
    pool->queue_len--;
    pthread_cond_broadcast(&pool->work_cond);
    pool_unlock();
  }

  /*

   * WARN: loop is only broken if pool->running is false, in that case pool
   *       would already be locked (see the start of the loop) so we don't need
   *       an extra lock here, and we should also unlock without returning

  */

  /* notify lp_pool_free() before quitting */
  pool->thread_count--;
  pthread_cond_signal(&pool->quit_cond);

  pool_unlock();
  return pool;
}

lp_pool_t *lp_pool_init(lp_pool_t *pool, uint32_t size) {
  pthread_t thread;

  /* check the pool size */
  if (size == 0) {
    errno = EINVAL;
    return NULL;
  }

  /* initialize the thread pool structure */
  if (NULL == pool)
    pool = calloc(1, sizeof(*pool));
  else
    memset(pool, 0, sizeof(*pool));

  pool->running = true;
  pthread_mutex_init(&pool->lock, NULL);
  pthread_cond_init(&pool->work_cond, NULL);
  pthread_cond_init(&pool->quit_cond, NULL);

  for (; size > 0; size--) {
    /* create the thread */
    if (pthread_create(&thread, NULL, _pool_worker, pool) != 0) {
      lp_pool_free(pool);
      return NULL;
    }

    /* deattach the thread */
    if (pthread_detach(thread) != 0) {
      lp_pool_free(pool);
      return NULL;
    }
  }

  return pool;
}

bool lp_pool_add(lp_pool_t *pool, lp_pool_func_t func, void *data) {
  struct lp_work *work = NULL;

  /* check the arguments */
  if (NULL == pool || NULL == func) {
    errno = EINVAL;
    return false;
  }

  /* allocate & setup the new work structure */
  if (NULL == (work = calloc(1, sizeof(struct lp_work))))
    return false;

  work->func = func;
  work->data = data;

  /* add work to the queue and broadcast to tell all the other threads */
  pool_lock();

  work->next  = pool->queue;
  pool->queue = work;
  pool->queue_len++;

  pthread_cond_broadcast(&pool->work_cond);

  pool_unlock();
  return true;
}

bool lp_pool_wait(lp_pool_t *pool, bool display) {
  /* TODO: implement bar for the display option */
  (void)display;

  pool_lock();

  while (pool->queue_len > 0)
    pthread_cond_wait(&pool->work_cond, &pool->lock);

  pool_unlock();
  return true;
}

void lp_pool_free(lp_pool_t *pool) {
  struct lp_work *cur = NULL, *next = NULL;

  /* clear the work queue and stop the pool */
  pool_lock();
  next = pool->queue;

  while (NULL != (cur = next)) {
    next = cur->next;
    free(cur);
  }

  pool->running = false;
  pthread_cond_broadcast(&pool->work_cond);
  pool_unlock();

  /* wait till all the threads are done */
  pool_lock();

  while (pool->thread_count > 0)
    pthread_cond_wait(&pool->quit_cond, &pool->lock);

  pool_unlock();

  /* free the resources of the thread pool */
  pthread_cond_destroy(&pool->work_cond);
  pthread_cond_destroy(&pool->quit_cond);
  pthread_mutex_destroy(&pool->lock);

  memset(pool, 0, sizeof(*pool));
}
