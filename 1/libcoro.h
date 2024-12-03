#pragma once

#include <stdbool.h>

struct coro;
typedef void *(*coro_f)(void *);

/** Initialize the coroutines engine. */
void
coro_sched_init(void);

/**
 * Run the coroutines processing while there are any runnable
 * ones.
 */
void
coro_sched_run(void);

/**
 * Destroy the coroutines engine. All coros must be finished by
 * now.
 */
void
coro_sched_destroy(void);

/** Get the currently working coroutine. */
struct coro *
coro_this(void);

/**
 * Create a new coroutine. The function won't yield. The coroutine
 * will start execution automatically on the next iteration of the
 * scheduler.
 *
 * Whatever the callback function returns, will be returned from
 * coro_join().
 */
struct coro *
coro_new(coro_f func, void *func_arg);

/**
 * Join a coroutine. When joined, its resources are freed, and the
 * result of its callback function is returned. Each coroutine
 * must be joined. Otherwise it leaks.
 */
void *
coro_join(struct coro *coro);

/**
 * Pause the current coroutine until its explicitly woken up with
 * coro_wakeup(). Can be used to wait for some event, which will
 * wakeup this coro when happens.
 */
void
coro_suspend(void);

/**
 * Pause the current coroutine until the next iteration of the
 * scheduler. Can be used to let the other coroutines work for a
 * bit.
 */
void
coro_yield(void);

/**
 * Wakeup a coroutine. If it was suspended, then it is going to be
 * continued on the next iteration of the scheduler. Otherwise
 * this function is a nop.
 */
void
coro_wakeup(struct coro *coro);
