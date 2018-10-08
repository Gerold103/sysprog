#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main()
{
	int fd = open("tmp.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	int fd2 = dup(fd);
	if (fork() == 0) {
		close(fd);
		dprintf(fd2, "%d ", (int) getpid());
		close(fd2);
		return 0;
	}
	close(fd2);
	wait(NULL);
	dprintf(fd, "%d ", (int) getpid());
	close(fd);
	return 0;
}
