#include <stdio.h>
#include <unistd.h>

int
main(int argc, const char **argv)
{
	pid_t id = getpgrp();
	pid_t pid = getpid();
	int i = 0;
	if (argv[1][0] == 'w')
		printf("1\n");
	else
		scanf("%d", &i);
	fprintf(stderr, "group = %d, pid = %d\n",
		(int) id, (int) pid);
	return 0;
}
