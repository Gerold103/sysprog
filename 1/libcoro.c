#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "libcoro.h"

#define handle_error() ({printf("Error %s\n", strerror(errno)); exit(-1);})

/** Main coroutine structure, its context. */
struct coro {
	/** A value, returned by func. */
	int ret;
	/** Stack, used by the coroutine. */
	void *stack;
	/** An argument for the function func. */
	void *func_arg;
	/** A function to call as a coroutine. */
	coro_f func;
	/** Last remembered coroutine context. */
	sigjmp_buf ctx;
	/** True, if the coroutine has finished. */
	bool is_finished;
	long long switch_count;
	/** Links in the coroutine list, used by scheduler. */
	struct coro *next, *prev;
};

/**
 * Scheduler is a main coroutine - it catches and returns dead
 * ones to a user.
 */
static struct coro coro_sched;
/**
 * True, if in that moment the scheduler is waiting for a
 * coroutine finish.
 */
static bool is_sched_waiting = false;
/** Which coroutine works at this moment. */
static struct coro *coro_this_ptr = NULL;
/** List of all the coroutines. */
static struct coro *coro_list = NULL;
/**
 * Buffer, used by the coroutine constructor to escape from the
 * signal handler back into the constructor to rollback
 * sigaltstack etc.
 */
static sigjmp_buf start_point;

/** Add a new coroutine to the beginning of the list. */
static void
coro_list_add(struct coro *c)
{
	c->next = coro_list;
	c->prev = NULL;
	if (coro_list != NULL)
		coro_list->prev = c;
	coro_list = c;
}

/** Remove a coroutine from the list. */
static void
coro_list_delete(struct coro *c)
{
	struct coro *prev = c->prev, *next = c->next;
	if (prev != NULL)
		prev->next = next;
	if (next != NULL)
		next->prev = prev;
	if (prev == NULL)
		coro_list = next;
}

int
coro_status(const struct coro *c)
{
	return c->ret;
}

long long
coro_switch_count(const struct coro *c)
{
	return c->switch_count;
}

bool
coro_is_finished(const struct coro *c)
{
	return c->is_finished;
}

void
coro_delete(struct coro *c)
{
	free(c->stack);
	free(c);
}

/** Switch the current coroutine to an arbitrary one. */
static void
coro_yield_to(struct coro *to)
{
	struct coro *from = coro_this_ptr;
	++from->switch_count;
	if (sigsetjmp(from->ctx, 0) == 0)
		siglongjmp(to->ctx, 1);
	coro_this_ptr = from;
}

void
coro_yield(void)
{
	struct coro *from = coro_this_ptr;
	struct coro *to = from->next;
	if (to == NULL)
		coro_yield_to(&coro_sched);
	else
		coro_yield_to(to);
}

void
coro_sched_init(void)
{
	memset(&coro_sched, 0, sizeof(coro_sched));
	coro_this_ptr = &coro_sched;
}

struct coro *
coro_sched_wait(void)
{
	while (coro_list != NULL) {
		for (struct coro *c = coro_list; c != NULL; c = c->next) {
			if (c->is_finished) {
				coro_list_delete(c);
				return c;
			}
		}
		is_sched_waiting = true;
		coro_yield_to(coro_list);
		is_sched_waiting = false;
	}
	return NULL;
}

struct coro *
coro_this(void)
{
	return coro_this_ptr;
}

/**
 * The core part of the coroutines creation - this signal handler
 * is run on a separate stack using sigaltstack. On an invokation
 * it remembers its current context and jumps back to the
 * coroutine constructor. Later the coroutine continues from here.
 */
static void
coro_body(int signum)
{
	(void)signum;
	struct coro *c = coro_this_ptr;
	coro_this_ptr = NULL;
	/*
	 * On an invokation jump back to the constructor right
	 * after remembering the context.
	 */
	if (sigsetjmp(c->ctx, 0) == 0)
		siglongjmp(start_point, 1);
	/*
	 * If the execution is here, then the coroutine should
	 * finaly start work.
	 */
	coro_this_ptr = c;
	c->ret = c->func(c->func_arg);
	c->is_finished = true;
	/* Can not return - 'ret' address is invalid already! */
	if (! is_sched_waiting) {
		printf("Critical error - no place to return!\n");
		exit(-1);
	}
	siglongjmp(coro_sched.ctx, 1);
}

struct coro *
coro_new(coro_f func, void *func_arg)
{
	struct coro *c = (struct coro *) malloc(sizeof(*c));
	c->ret = 0;
	int stack_size = 1024 * 1024;
	if (stack_size < SIGSTKSZ)
		stack_size = SIGSTKSZ;
	c->stack = malloc(stack_size);
	c->func = func;
	c->func_arg = func_arg;
	c->is_finished = false;
	c->switch_count = 0;
	/*
	 * SIGUSR2 is used. First of all, block new signals to be
	 * able to set a new handler.
	 */
	sigset_t news, olds, suss;
	sigemptyset(&news);
	sigaddset(&news, SIGUSR2);
	if (sigprocmask(SIG_BLOCK, &news, &olds) != 0)
		handle_error();
	/*
	 * New handler should jump onto a new stack and remember
	 * that position. Afterwards the stack is disabled and
	 * becomes dedicated to that single coroutine.
	 */
	struct sigaction newsa, oldsa;
	newsa.sa_handler = coro_body;
	newsa.sa_flags = SA_ONSTACK;
	sigemptyset(&newsa.sa_mask);
	if (sigaction(SIGUSR2, &newsa, &oldsa) != 0)
		handle_error();
	/* Create that new stack. */
	stack_t oldst, newst;
	newst.ss_sp = c->stack;
	newst.ss_size = stack_size;
	newst.ss_flags = 0;
	if (sigaltstack(&newst, &oldst) != 0)
		handle_error();
	/* Jump onto the stack and remember its position. */
	struct coro *old_this = coro_this_ptr;
	coro_this_ptr = c;
	sigemptyset(&suss);
	if (sigsetjmp(start_point, 1) == 0) {
		raise(SIGUSR2);
		while (coro_this_ptr != NULL)
			sigsuspend(&suss);
	}
	coro_this_ptr = old_this;
	/*
	 * Return the old stack, unblock SIGUSR2. In other words,
	 * rollback all global changes. The newly created stack
	 * now is remembered only by the new coroutine, and can be
	 * used by it only.
	 */
	if (sigaltstack(NULL, &newst) != 0)
		handle_error();
	newst.ss_flags = SS_DISABLE;
	if (sigaltstack(&newst, NULL) != 0)
		handle_error();
	if ((oldst.ss_flags & SS_DISABLE) == 0 &&
	    sigaltstack(&oldst, NULL) != 0)
		handle_error();
	if (sigaction(SIGUSR2, &oldsa, NULL) != 0)
		handle_error();
	if (sigprocmask(SIG_SETMASK, &olds, NULL) != 0)
		handle_error();

	/* Now scheduler can work with that coroutine. */
	coro_list_add(c);
	return c;
}
