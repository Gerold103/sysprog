#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdbool.h>
#include "clone.h"

typedef int (*cthread_f)(void *);

struct cthread {
	int returned_code;
	cthread_f func;
	void *arg;
	void *stack;
	bool is_finished;
};

int
cthread_runner(void *arg)
{
	struct cthread *thread = (struct cthread *) arg;
	thread->returned_code = thread->func(thread->arg);
	thread->is_finished = true;
	return 0;
}

void
cthread_create(struct cthread *result, cthread_f func,
	       void *arg)
{
	result->returned_code = 0;
	result->func = func;
	result->arg = arg;
	result->is_finished = false;
	thread_create_clone(cthread_runner, (void *) result,
			    &result->stack);
}

int
cthread_join(volatile struct cthread *thread)
{
	while (! thread->is_finished)
		sched_yield();
	free(thread->stack);
	return thread->returned_code;
}

int
func(void *arg)
{
	printf("arg = %d\n", *((int *) arg));
	return 200;
}

int
main()
{
	struct cthread thread;
	int arg = 100;
	cthread_create(&thread, func, (void *) &arg);
	int retcode = cthread_join(&thread);
	printf("thread is joined with retcode = %d\n", retcode);
	return 0;
}
