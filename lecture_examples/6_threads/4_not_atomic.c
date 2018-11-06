#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "clone.h"

volatile bool is_finished = false;
volatile int counter = 0;

int
thread_f(void *arg)
{
	for (int i = 0; i < 100000; ++i)
		counter = counter + 1;
	is_finished = true;
	return 0;
}

int
main()
{
	void *stack;
	thread_create_clone(thread_f, NULL, &stack);
	for (int i = 0; i < 100000; ++i)
		counter = counter + 1;
	while (! is_finished)
		sched_yield();
	printf("counter = %d\n", counter);
	free(stack);
	return 0;
}
