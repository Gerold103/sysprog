#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>

static inline int
thread_create_clone(int (*func)(void *), void *arg, void **stack)
{
	int stack_size = 65 * 1024;
	*stack = malloc(stack_size);
	void *stack_top = (char *) *stack + stack_size;
	int flags = CLONE_VM | CLONE_FS | CLONE_FILES |
		    CLONE_SIGHAND | CLONE_THREAD;
	return clone(func, stack_top, flags, arg);
}

static inline int
thread_create_clone_tid(int (*func)(void *), void *arg, void **stack,
			pid_t *tid)
{
	int stack_size = 65 * 1024;
	*stack = malloc(stack_size);
	void *stack_top = (char *) *stack + stack_size;
	int flags = CLONE_VM | CLONE_FS | CLONE_FILES |
		    CLONE_SIGHAND | CLONE_THREAD | CLONE_CHILD_CLEARTID |
		    CLONE_CHILD_SETTID;
	return clone(func, stack_top, flags, arg, NULL, NULL, tid);
}
