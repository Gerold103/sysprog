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
		return execlp("python3", "python3", "-i", NULL);
	}
	close(fd_to_child[0]);
	const char cmd[] = "print(100 + 200)";
	write(fd_to_child[1], cmd, sizeof(cmd));
	close(fd_to_child[1]);
	wait(NULL);
	return 0;
}
