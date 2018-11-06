#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdbool.h>
#include <setjmp.h>
#include "clone.h"

typedef int (*cthread_f)(void *);

struct cthread {
	int returned_code;
	cthread_f func;
	void *arg;
	void *stack;
	bool is_finished;
	bool is_detached;
	jmp_buf jmp;
};

void
cthread_destroy(struct cthread *thread)
{
	printf("thread is destroyed\n");
	free(thread->stack);
}

int
cthread_join(volatile struct cthread *thread)
{
	printf("thread is joined\n");
	while (! thread->is_finished)
		sched_yield();
	cthread_destroy((struct cthread *) thread);
	return thread->returned_code;
}

int
cthread_runner(void *arg)
{
	struct cthread *thread = (struct cthread *) arg;
	if (setjmp(thread->jmp) == 0) {
		thread->returned_code =
			thread->func(thread->arg);
	}
	if (thread->is_detached)
		cthread_destroy(thread);
	thread->is_finished = true;
	return 0;
}

void
cthread_detach(struct cthread *thread)
{
	if (thread->is_finished)
		cthread_destroy(thread);
	sleep(1);
	thread->is_detached = true;
}

void
cthread_create(struct cthread *result, cthread_f func,
	       void *arg)
{
	result->returned_code = 0;
	result->func = func;
	result->arg = arg;
	result->is_finished = false;
	result->is_detached = false;
	thread_create_clone(cthread_runner, (void *) result,
			    &result->stack);
}

void
cthread_exit(struct cthread *thread, int retcode)
{
	thread->returned_code = retcode;
	longjmp(thread->jmp, 1);
}

int
func(void *arg)
{
	printf("thread started\n");
	return 0;
}

int
main()
{
	struct cthread thread;
	cthread_create(&thread, func, NULL);
	cthread_detach(&thread);
	while (! thread.is_finished)
		sched_yield();
	printf("detached thread finished\n");
	return 0;
}
