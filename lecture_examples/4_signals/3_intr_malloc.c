#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

static int alarm_cnt = 0;

static void
do_malloc(void)
{
	for (int i = 0; i < 10; ++i)
		free(malloc(1024 * 1024 * i));
}

static void
on_new_signal(int signum)
{
	printf("Interrupted %d times\n", ++alarm_cnt);
	do_malloc();
}

int
main(void)
{
	printf("my pid: %d\n", (int) getpid());
	signal(SIGUSR1, on_new_signal);
	while (true)
		do_malloc();
	return 0;
}
