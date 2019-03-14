#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main()
{
	int to_child[2];
	pipe(to_child);
	char buf[16];
	dup2(to_child[0], 0);
	if (fork() == 0) {
		close(to_child[1]);
		return execlp("python3", "python3", "-i", NULL);
	}
	close(to_child[0]);
	const char cmd[] = "print(100 + 200)";
	write(to_child[1], cmd, sizeof(cmd));
	close(to_child[1]);
	wait(NULL);
	return 0;
}
