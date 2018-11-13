#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

int main()
{
	int fd = open("/tmp/fifo_server", O_WRONLY);
	pid_t pid = getpid();
	write(fd, &pid, sizeof(pid));
	printf("my pid %d is sent to server\n", (int) pid);
	close(fd);
	return 0;
}
