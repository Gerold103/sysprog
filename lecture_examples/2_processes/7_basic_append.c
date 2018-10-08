#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
	int fd = open("tmp.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	pid_t pid = getpid();
	printf("press any key to write my pid %d\n", (int) pid);
	getchar();

	dprintf(fd, "%d ", (int) pid);
	close(fd);
	return 0;
}
