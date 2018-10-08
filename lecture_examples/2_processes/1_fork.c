#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	printf("I am process %d\n", (int) getpid());
	char *mem = (char *) calloc(1, 100);
	pid_t child_pid = fork();
	pid_t my_pid = getpid();
	if (child_pid == 0) {
		printf("%d: I am child, fork returned %d\n",
		       (int) my_pid, (int) child_pid);
		printf("%d: child is terminated with code 100\n",
		       (int) my_pid);
		printf("%d: memory values are set to 1\n", (int) my_pid);
		memset(mem, 1, 100);
		return 100;
	}
	printf("%d: I am parent, fork returned %d\n",
	       (int) my_pid, (int) child_pid);
	int stat;
	pid_t wait_result = wait(&stat);
	printf("%d: wait returned %d and stat %d\n", (int) my_pid,
	       (int) wait_result, stat);
	printf("%d: memory values are %d\n", (int) my_pid, (int) mem[0]);
	printf("%d: returned child code was %d\n", (int) my_pid,
	       WEXITSTATUS(stat));
	return 0;
}
