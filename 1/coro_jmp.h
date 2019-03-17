#include <setjmp.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#ifndef CORO_LOCAL_DATA
#error "You are expected to specify what you want to store in a coroutine "\
       "via CORO_LOCAL_DATA"
#endif

/**
 * Coroutines library. It allows to split execution of a task
 * into a set of coroutines. Possible example of usage:
 *
 *
 * foreach (coro : coros)
 *     coro_init(coro);
 * coro_call(func_to_split);
 *
 *
 * void other_func1()
 * {
 *     ...
 *     coro_return();
 * }
 *
 * void other_func2()
 * {
 *     coro_call(func1);
 *     ...
 *     coro_return();
 * }
 *
 * void func_to_split()
 * {
 *     ...
 *     coro_call(other_func1);
 *     ...
 *     coro_yield();
 *     ...
 *     coro_call(other_func2);
 *     ...
 *     coro_finish();
 *     coro_wait_all();
 * }
 */

/**
 * This struct describes one single coroutine. It stores its
 * local variables and a position, where it stands now.
 */
struct coro {
	/**
	 * Important thing - execution position where that
	 * coroutine stopped last time.
	 */
	jmp_buf exec_point;
	/**
	 * Stack of points remembered before a call of a function
	 * from the coroutine. Before each new call stack position
	 * is remembered here, and the function returns here via
	 * longjmp.
	 */
	jmp_buf *ret_points;
	/** Number of used points. */
	int ret_count;
	/** Maximal number of points to store. */
	int ret_capacity;
	/**
	 * This flag is set when the coroutine has finished its
	 * task. It is used to wait until all the coroutines are
	 * finished.
	 */
	bool is_finished;
	CORO_LOCAL_DATA;
};

/**
 * In your code it should be dynamic, not const. Here it is 3 for
 * simplicity.
 */
#define coro_count 3
static struct coro coros[coro_count];

/**
 * Index of the currently working coroutine. It is used to learn
 * which coroutine should be scheduled next, and to get your
 * current local variables.
 */
static int curr_coro_i = 0;

/** Get currently working coroutine. */
#define coro_this() (&coros[curr_coro_i])

/** Declare that this curoutine has finished. */
#define coro_finish() ({ coro_this()->is_finished = true; })

/**
 * This macro stops the current coroutine and switches to another
 * one. Check is not in a function, because setjmp result can not
 * be used after 'return'. You should keep it as macros. In your
 * code instead of real call and 'return' use coro_call() and
 * coro_return().
 */
#define coro_yield() ({						\
	int old_i = curr_coro_i;				\
	curr_coro_i = (curr_coro_i + 1) % coro_count;		\
	if (setjmp(coros[old_i].exec_point) == 0)		\
		longjmp(coros[curr_coro_i].exec_point, 1);	\
})

/** Initialize a coroutine. */
#define coro_init(coro) ({					\
	(coro)->is_finished = false;				\
	(coro)->ret_count = 0;					\
	(coro)->ret_capacity = 0;				\
	(coro)->ret_points = NULL;				\
	setjmp((coro)->exec_point);				\
})

/**
 * Call a function, but do it safely, creating a point to jump
 * back from that function, instead of 'return'.
 */
#define coro_call(func, ...) ({					\
	struct coro *c = coro_this();				\
	if (c->ret_count + 1 > c->ret_capacity) {		\
		int new_cap = (c->ret_capacity + 1) * 2;	\
		int new_size = new_cap * sizeof(jmp_buf);	\
		c->ret_points =					\
			(jmp_buf *) realloc(c->ret_points,	\
					    new_size);		\
		assert(c->ret_points != NULL);			\
		c->ret_capacity = new_cap;			\
	}							\
	if (setjmp(c->ret_points[c->ret_count]) == 0) {		\
		++c->ret_count;					\
		func(__VA_ARGS__);				\
	}							\
})

/**
 * Return from a function, previously called via coro_call().
 * Tricky thing - it does not use 'return', because it taints
 * all the jmp_buf's. Instead, it jumps out of that function.
 */
#define coro_return() ({					\
	struct coro *c = coro_this();				\
	longjmp(c->ret_points[--c->ret_count], 1);		\
})

/** Wait until all the coroutines have finished. */
#define coro_wait_all() do {					\
	bool is_all_finished = true;				\
	for (int i = 0; i < coro_count; ++i) {			\
		if (! coros[i].is_finished) {			\
			fprintf(stderr, "Coro %d: still active,"\
				" re-scheduling\n", i);		\
			is_all_finished = false;		\
			break;					\
		}						\
	}							\
	if (is_all_finished) {					\
		fprintf(stderr, "No more active coros to "	\
			"schedule.\n");				\
		break;						\
	}							\
	coro_yield();						\
} while (true)
