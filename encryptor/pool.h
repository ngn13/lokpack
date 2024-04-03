/*
 *
 * !!THIS NOT PART OF KEYDOT!!
 * ========================================
 * See https://github.com/Pithikos/C-Thread-Pool
 * Licensed under MIT
 *
 */

#pragma once
#define THPOOL_THREAD_NAME thpool

typedef struct thpool_ *threadpool;
threadpool thpool_init(int);
int thpool_add_work(threadpool, void (*function_p)(void *), void *);
void thpool_wait(threadpool);
void thpool_pause(threadpool);
void thpool_resume(threadpool);
void thpool_destroy(threadpool);
int thpool_num_threads_working(threadpool);
