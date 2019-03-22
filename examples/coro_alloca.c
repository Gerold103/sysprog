#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>

enum {
	/** Total number of coroutines. 3 for simplicity. */
	CORO_COUNT = 3,
	/**
	 * This stack size is really small, especially for Mac.
	 * But enough for tiny functions.
	 */
	CORO_STACK = 8 * 1024,
};

/** Coroutine. */
struct coro {
	/** Last stop context. */
	jmp_buf ctx;
	/** True, if it has finished. */
	bool is_finished;
};

/** Currently working coroutine. */
static int coro_i;
/** Coroutines array. Fixed size for simplicity. */
static struct coro coros[CORO_COUNT];

/**
 * That macro allocates some stack and plans to start a coroutine
 * here. An actual start happens on coro_continue_next from the
 * main function.
 */
#define coro_put_here(c, func, ...) ({			\
	int rc = setjmp((c)->ctx);			\
	if (rc == 0) {					\
		(c)->is_finished = false;		\
		volatile char *p = alloca(CORO_STACK);	\
	} else {					\
		func(__VA_ARGS__);			\
		(c)->is_finished = true;		\
	}						\
	rc;						\
})

/** This coroutine. */
#define coro_this() (&coros[coro_i])

/**
 * Give the control to a next not finished coroutine. If all the
 * others are finished - do nothing.
 */
#define coro_yield() ({						\
	for (int i = 0; i < CORO_COUNT; ++i) {			\
		int next = (coro_i + 1) % CORO_COUNT;		\
		if (! coros[next].is_finished) {		\
			if (setjmp(coro_this()->ctx) == 0) {	\
				coro_i = next;			\
				longjmp(coro_this()->ctx, 1);	\
			}					\
			break;					\
		}						\
	}							\
})

/**
 * Continue a next not finished coroutine. Note, that function
 * does not return, if a not finished coroutine has been found.
 */
static void
coro_continue_next()
{
	for (int i = 0; i < CORO_COUNT; ++i) {
		if (! coros[i].is_finished) {
			coro_i = i;
			longjmp(coro_this()->ctx, 1);
		}
	}
}

/**
 * Just a function to test the example. It calls itself
 * recursively and yields many times.
 */
static void
coro_example_func(int deep)
{
	printf("%d: start deep = %d\n", coro_i, deep);
	coro_yield();
	if (deep < 2)
		coro_example_func(deep + 1);
	coro_yield();
	printf("%d: end deep = %d\n", coro_i, deep);
}

int
main(void)
{
	printf("Start main\n");
	for (int i = 0; i < CORO_COUNT; ++i) {
		if (coro_put_here(&coros[i], coro_example_func, 0) != 0)
			break;
	}
	/* Repeat until no next unfinished coros. */
	coro_continue_next();
	printf("Finish main\n");
	return 0;
}
