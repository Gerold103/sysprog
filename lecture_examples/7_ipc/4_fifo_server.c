#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sched.h>

int main()
{
	mkfifo("/tmp/fifo_server", S_IRWXU | S_IRWXO);
	int fd = open("/tmp/fifo_server", O_RDONLY);
	while (1) {
		pid_t new_client;
		if (read(fd, &new_client, sizeof(pid_t)) > 0)
			printf("new client %d\n", (int) new_client);
		else
			sched_yield();
	}
}
