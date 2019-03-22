#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>
#include <setjmp.h>

static int finished = 0;
static jmp_buf buf;

static int
is_usr1_blocked(void)
{
	sigset_t old;
	sigprocmask(0, NULL, &old);
	return sigismember(&old, SIGUSR1);
}

static void
on_new_signal(int signum)
{
	printf("Process signal, usr1 block = %d\n", is_usr1_blocked());
	++finished;
	longjmp(buf, 1);
}

int
main(void)
{
	struct sigaction act;
	act.sa_handler = on_new_signal;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGUSR1, &act, NULL);
	printf("Before raise, usr1 block = %d\n", is_usr1_blocked());
	if (setjmp(buf) == 0)
		raise(SIGUSR1);
	while (finished != 1)
		sched_yield();
	printf("After raise, usr1 block = %d\n", is_usr1_blocked());
	return 0;
}
