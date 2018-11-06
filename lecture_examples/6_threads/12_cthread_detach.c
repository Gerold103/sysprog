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

void
spin_lock(volatile bool *lock)
{
	while (! __sync_bool_compare_and_swap(lock, 0, 1)) {};
}

void
spin_unlock(volatile bool *lock)
{
	__sync_bool_compare_and_swap(lock, 1, 0);
}

struct cthread_stack {
	pid_t tid;
	void *stack;
	struct cthread_stack *next;
};

struct cthread_stack *stack_list = NULL;
volatile bool last_stack_lock = false;

struct cthread {
	int returned_code;
	cthread_f func;
	void *arg;
	struct cthread_stack *stack;
	bool lock;
	bool is_finished;
	bool is_detached;
	jmp_buf jmp;
};

void
cthread_destroy(struct cthread *thread)
{
	printf("thread is destroyed\n");
	spin_lock(&last_stack_lock);
	struct cthread_stack *iter = stack_list;
	while (iter != NULL) {
		if (iter->tid != 0)
			break;
		struct cthread_stack *next = iter->next;
		free(iter->stack);
		free(iter);
		iter = next;
		printf("a stack is freed\n");
	}
	thread->stack->next = iter;
	stack_list = thread->stack;
	spin_unlock(&last_stack_lock);
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
	spin_lock(&thread->lock);
	if (thread->is_detached)
		cthread_destroy(thread);
	thread->is_finished = true;
	spin_unlock(&thread->lock);
	return 0;
}

void
cthread_detach(struct cthread *thread)
{
	spin_lock(&thread->lock);
	if (thread->is_finished)
		cthread_destroy(thread);
	//sleep(1);
	thread->is_detached = true;
	spin_unlock(&thread->lock);
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
	result->lock = false;
	result->stack = (struct cthread_stack *) malloc(sizeof(*result->stack));
	result->stack->next = NULL;
	thread_create_clone_tid(cthread_runner, (void *) result,
				&result->stack->stack, &result->stack->tid);
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
	struct cthread thread[10];
	for (int i = 0; i < 10; ++i) {
		cthread_create(&thread[i], func, NULL);
		cthread_detach(&thread[i]);
	}
	
	for (int i = 0; i < 10; ++i) {
		if (! thread[i].is_finished) {
			i = 0;
			sched_yield();
		}
	}
	printf("detached threads finished\n");

	return 0;
}
