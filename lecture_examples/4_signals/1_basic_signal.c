#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

static void
on_new_signal(int signum)
{
	switch(signum) {
	case SIGINT:
		printf("caught sigint\n");
		break;
	case SIGUSR1:
		printf("caught usr1\n");
		break;
	case SIGUSR2:
		printf("caught usr2\n");
		break;
	default:
		printf("caught unknown signal\n");
		exit(-1);
	}
}

int
main(void)
{
	printf("my pid: %d\n", (int) getpid());
	signal(SIGINT, on_new_signal);
	signal(SIGUSR1, on_new_signal);
	signal(SIGUSR2, on_new_signal);
	while(true) pause();
	return 0;
}
