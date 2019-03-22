#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include "12_libcoro.h"

#define coro_count 3
static struct coro *coros[coro_count];

static void
recursive(int deep, int id)
{
	printf("%d: recursive step %d\n", id, deep);
	coro_yield();
	if (deep < 2)
		recursive(deep + 1, id);
	printf("%d: finish recursive step %d\n", id, deep);
}

static int
coro_func(void *ptr)
{
	int id = (int) ptr;
	printf("%d: coro is started\n", id);
	coro_yield();
	recursive(0, id);
	return id;
}

static int
coro_tree_func(void *ptr)
{
	int id = (int) ptr;
	printf("%d: coro line is started\n", id);
	coro_yield();
	if (id > 1) {
		printf("%d: coro line end\n", id);
	} else {
		printf("%d: coro line next\n", id);
		coro_new(coro_tree_func, (void *) (id + 1));
	}
	coro_yield();
	return id;
}

int
main(void)
{
	printf("Start main\n");
	coro_sched_init();
	for (int i = 0; i < coro_count; ++i)
		coros[i] = coro_new(coro_func, (void *) i);
	struct coro *c;
	while ((c = coro_sched_wait()) != NULL) {
		printf("Finished %d\n", coro_status(c));
		coro_delete(c);
	}
	printf("Finished simple\n");

	coro_new(coro_tree_func, (void *) 0);
	while ((c = coro_sched_wait()) != NULL) {
		printf("Finished %d\n", coro_status(c));
		coro_delete(c);
	}
	printf("Finish main\n");
	return 0;
}
