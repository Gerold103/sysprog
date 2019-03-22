#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static int cnt = 0;

static void
on_new_signal(int signum)
{
	printf("Processed a signal %dth time\n", ++cnt);
}

int
main(void)
{
	printf("Start\n");
	signal(SIGINT, on_new_signal);
	sigset_t oldm, newm, pendingm;
	sigemptyset(&newm);
	sigaddset(&newm, SIGINT);
	printf("Block SIGINT\n");
	sigprocmask(SIG_BLOCK, &newm, &oldm);
	getchar();
	sigpending(&pendingm);
	if (sigismember(&pendingm, SIGINT))
		printf("SIGINT is pending\n");
	printf("Unblock SIGINT\n");
	sigprocmask(SIG_SETMASK, &oldm, NULL);
	while(cnt < 4)
		pause();
	return 0;
}
