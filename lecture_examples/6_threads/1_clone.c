#include "clone.h"
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdbool.h>

pid_t
gettid()
{
	return syscall(SYS_gettid);
}

volatile bool is_finished = false;

int
thread_f(void *arg)
{
	pid_t thread_id = gettid();
	pid_t pid = getpid();
	printf("pid %d, tid %d: new thread, arg = %d\n",
	       (int)pid, (int)thread_id, *((int *) arg));
	is_finished = true;
	return 0;
}

int
main()
{
	pid_t thread_id = gettid();
	pid_t pid = getpid();
	printf("pid %d, tid %d: main thread\n",
	       (int)pid, (int)thread_id);
	void *stack;
	int arg = 100;
	int ret = thread_create_clone(thread_f, (void *) &arg, &stack);
	printf("pid %d, tid %d: clone result = %d\n",
	       (int)pid, (int)thread_id, ret);
	while (! is_finished)
		sched_yield();
	free(stack);
	return 0;
}
