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
	jmp_buf jmp;
};

int
cthread_runner(void *arg)
{
	struct cthread *thread = (struct cthread *) arg;
	if (setjmp(thread->jmp) == 0) {
		thread->returned_code =
			thread->func(thread->arg);
	}
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

void
cthread_exit(struct cthread *thread, int retcode)
{
	thread->returned_code = retcode;
	longjmp(thread->jmp, 1);
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
	printf("func is started\n");
	struct cthread *me = (struct cthread *) arg;
	cthread_exit(me, 300);
	printf("func returned\n");
	return 0;
}

int
main()
{
	struct cthread thread;
	cthread_create(&thread, func, (void *) &thread);
	int retcode = cthread_join(&thread);
	printf("thread is joined with retcode = %d\n", retcode);
	return 0;
}
