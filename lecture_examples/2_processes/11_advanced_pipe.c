#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main()
{
	int fd_to_child[2];
	pipe(fd_to_child);
	char buf[16];
	dup2(fd_to_child[0], 0);
	if (fork() == 0) {
		close(fd_to_child[1]);
		int n;
		scanf("%d", &n);
		printf("%d: read %d\n", (int) getpid(), n);
		return 0;
	}
	close(fd_to_child[0]);
	write(fd_to_child[1], "100", sizeof("100"));
	wait(NULL);
	return 0;
}
