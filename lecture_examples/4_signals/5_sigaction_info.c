#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static void
on_new_signal(int signum, siginfo_t *info, void *context)
{
	printf("Segfault: signum = %d, si_signo = %d, si_addr = %p, si_code = \n",
	       signum, info->si_signo, info->si_addr);
	switch (info->si_code) {
	case SEGV_MAPERR:
		printf("not mapped\n");
		break;
	case SEGV_ACCERR:
		printf("no permission\n");
		break;
	default:
		printf("%d\n", info->si_code);
		break;
	}
	exit(-1);
}

int
main(void)
{
	struct sigaction act;
	act.sa_sigaction = on_new_signal;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &act, NULL);
	char *ptr = (char *) 10;
	*ptr = 100;
	return 0;
}
