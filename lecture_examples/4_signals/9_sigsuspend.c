#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static int finished = 0;

static int
is_int_blocked(void)
{
	sigset_t old;
	sigprocmask(0, NULL, &old);
	return sigismember(&old, SIGINT);
}

static void
on_new_signal(int signum)
{
	printf("Process signal, int block = %d\n", is_int_blocked());
	++finished;
}

int
main(void)
{
	printf("Before main, int block = %d\n", is_int_blocked());
	sigset_t block, old;
	sigemptyset(&block);
	sigaddset(&block, SIGINT);
	sigprocmask(SIG_BLOCK, &block, &old);

	struct sigaction act;
	act.sa_handler = on_new_signal;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);
	sigemptyset(&block);
	while (finished != 1)
		sigsuspend(&block);
	sigprocmask(SIG_SETMASK, &old, NULL);
	printf("After main, int block = %d\n", is_int_blocked());
	return 0;
}
