#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

static bool is_sig_received = false;

static void
on_new_signal(int signum)
{
	printf("Received a signal\n");
	is_sig_received = true;
}

static void
interrupt(void)
{
	getchar();
}

int
main(void)
{
	printf("my pid: %d\n", (int) getpid());
	signal(SIGUSR1, on_new_signal);
	printf("Installed a handler\n");
	interrupt();
	while (! is_sig_received) {
		interrupt();
		printf("Wait for a signal\n");
		pause();
	}
	printf("Finish\n");
	return 0;
}
