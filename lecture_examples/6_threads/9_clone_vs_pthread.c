#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include "clone.h"

volatile bool is_finished = false;

volatile pthread_t child_id;

int
thread_f(void *arg)
{
	child_id = pthread_self();
	is_finished = true;
	return 0;
}

int
main()
{
	void *stack;
	thread_create_clone(thread_f, NULL, &stack);

	while (! is_finished)
		sched_yield();
	
	pthread_join(child_id, NULL);
	free(stack);
	return 0;
}
