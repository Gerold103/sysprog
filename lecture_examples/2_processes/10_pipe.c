#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main()
{
	int fd_to_parent[2];
	int fd_to_child[2];
	pipe(fd_to_child);
	pipe(fd_to_parent);
	char buf[16];
	if (fork() == 0) {
		close(fd_to_parent[0]);
		close(fd_to_child[1]);
		read(fd_to_child[0], buf, sizeof(buf));
		printf("%d: read %s\n", (int) getpid(), buf);
		write(fd_to_parent[1], "hello2", sizeof("hello2"));
		return 0;
	}
	close(fd_to_parent[1]);
	close(fd_to_child[0]);
	write(fd_to_child[1], "hello1", sizeof("hello"));
	read(fd_to_parent[0], buf, sizeof(buf));
	printf("%d: read %s\n", (int) getpid(), buf);
	wait(NULL);
	return 0;
}
