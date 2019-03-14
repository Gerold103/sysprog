#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main()
{
	int to_parent[2];
	int to_child[2];
	pipe(to_child);
	pipe(to_parent);
	char buf[16];
	if (fork() == 0) {
		close(to_parent[0]);
		close(to_child[1]);
		read(to_child[0], buf, sizeof(buf));
		printf("%d: read %s\n", (int) getpid(), buf);
		write(to_parent[1], "hello2", sizeof("hello2"));
		return 0;
	}
	close(to_parent[1]);
	close(to_child[0]);
	write(to_child[1], "hello1", sizeof("hello"));
	read(to_parent[0], buf, sizeof(buf));
	printf("%d: read %s\n", (int) getpid(), buf);
	wait(NULL);
	return 0;
}
