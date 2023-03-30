#pragma once

#include <stdbool.h>

struct coro;
typedef int (*coro_f)(void *);

/** Make current context scheduler. */
void
coro_sched_init(void);

/**
 * Block until any coroutine has finished. It is returned. NULl,
 * if no coroutines.
 */
struct coro *
coro_sched_wait(void);

/** Currently working coroutine. */
struct coro *
coro_this(void);

/**
 * Create a new coroutine. It is not started, just added to the
 * scheduler.
 */
struct coro *
coro_new(coro_f func, void *func_arg);

/** Return status of the coroutine. */
int
coro_status(const struct coro *c);

long long
coro_switch_count(const struct coro *c);

/** Check if the coroutine has finished. */
bool
coro_is_finished(const struct coro *c);

/** Free coroutine stack and it itself. */
void
coro_delete(struct coro *c);

/** Switch to another not finished coroutine. */
void
coro_yield(void);
