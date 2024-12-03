#pragma once

#include <stdbool.h>

struct coro;
typedef void * (*coro_f)(void *);

void
coro_sched_init(void);

void
coro_sched_run(void);

void
coro_sched_destroy(void);

/** Currently working coroutine. */
struct coro *
coro_this(void);

/**
 * Create a new coroutine. It is not started, just added to the
 * scheduler.
 */
struct coro *
coro_new(coro_f func, void *func_arg);

void *
coro_join(struct coro *coro);

void
coro_suspend(void);

void
coro_yield(void);

void
coro_wakeup(struct coro *coro);
