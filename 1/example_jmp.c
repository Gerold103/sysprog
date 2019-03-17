#define CORO_LOCAL_DATA struct {				\
	int deep;						\
	char local_data[128];					\
	int arg;						\
}
#include "coro_jmp.h"

/**
 * You can compile and run this example using the commands:
 *
 * $> gcc example_jmp.c
 * $> ./a.out
 */

/**
 * A function, called from inside of coroutines, and even
 * recursively.
 */
static void
other_function(int arg)
{
	printf("Coro %d: entered function, deep = %d, arg = %d\n", curr_coro_i,
	       coro_this()->deep, arg);
	coro_this()->arg = arg;
	coro_yield();
	printf("Coro %d: after yield arg = %d, but coro_this()->arg = %d\n",
		curr_coro_i, arg, coro_this()->arg);
	/*
	 * Here I've decided to call it recursively 2 times in
	 * each coro.
	 */
	if (++coro_this()->deep < 2)
		coro_call(other_function, curr_coro_i * 10);
	coro_return();
}

/**
 * Coroutine body. This code is executed by all the corutines at
 * the same time, but with different struct coro. Here you
 * implement your solution.
 */
static void
my_coroutine()
{
	/*
	 * Note - all the data access is done via 'coro_this()'.
	 * It is not safe to store anything in local variables
	 * here.
	 */
	sprintf(coro_this()->local_data, "Local data for coro id%d",
		curr_coro_i);
	fprintf(stderr, "Coro %d: before re-schedule\n", curr_coro_i);
	coro_yield();
	fprintf(stderr, "Coro %d: after first re-schedule\n", curr_coro_i);
	coro_yield();
	fprintf(stderr, "Coro %d: after second re-schedule\n", curr_coro_i);
	/* Other functions can be called, but via coro_call(). */
	coro_call(other_function, curr_coro_i * 10);
	fprintf(stderr, "Coro %d: this is local data: %s\n", curr_coro_i,
		coro_this()->local_data);
	coro_finish();
	coro_wait_all();
}

int
main(int argc, char **argv)
{
	for (int i = 0; i < coro_count; ++i) {
		if (coro_init(&coros[i]) != 0)
			break;
	}
	coro_call(my_coroutine);
	printf("Finished\n");
	return 0;
}
