#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>
#include <errno.h>

static int finished = 0;

#define handle_error() ({printf("error = %s\n", strerror(errno)); exit(-1); })

static void
on_new_signal(int signum)
{
	volatile int local_var;
	printf("Process signal, stack = %p\n", &local_var);
	++finished;
}

static void
stack_create(int size)
{
	int page_size = getpagesize();
	size = size + (page_size - size % page_size);
	if (size < SIGSTKSZ)
		size = SIGSTKSZ;
	stack_t s;
	s.ss_sp = malloc(size);
	printf("Page size = %d, new stack begin = %p, stack end = %p\n",
	       page_size, s.ss_sp, s.ss_sp + size);
	s.ss_flags = 0;
	s.ss_size = size;
	if (sigaltstack(&s, NULL) != 0)
		handle_error();
}

static void
stack_destroy(void)
{
	stack_t s;
	if (sigaltstack(NULL, &s) != 0)
		handle_error();
	s.ss_flags = SS_DISABLE;
	if (sigaltstack(&s, NULL) != 0)
		handle_error();
	printf("Destroy stack %p\n", s.ss_sp);
	free(s.ss_sp);
}

static void
wait_signal(void)
{
	int need = finished + 1;
	raise(SIGUSR1);
	while (finished < need)
		sched_yield();
}

int
main(int argc, char **argv)
{
	struct sigaction act;
	printf("Main, stack = %p\n", &act);
	act.sa_handler = on_new_signal;
	sigemptyset(&act.sa_mask);

	act.sa_flags = SA_ONSTACK;
	stack_create(1024 * 64);
	if (sigaction(SIGUSR1, &act, NULL) != 0)
		handle_error();
	wait_signal();
	stack_destroy();

	act.sa_flags = 0;
	if (sigaction(SIGUSR1, &act, NULL) != 0)
		handle_error();
	wait_signal();
	return 0;
}
