#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

int
main(int argc, const char **argv)
{
	int channel[2];
	int need_close = argc > 1 && strcmp(argv[1], "close") == 0;
	pipe(channel);
	if (fork() == 0) {
		char buf[16];
		printf("child: started\n");
		if (need_close) {
			printf("child: close output channel\n");
			close(channel[1]);
		}
		while (read(channel[0], buf, sizeof(buf)) > 0)
			printf("child: read %s\n", buf);
		printf("child: EOF\n");
		return 0;
	}
	write(channel[1], "100", 3);
	printf("parent: written 100\n");
	if (need_close) {
		printf("parent: close output channel\n");
		close(channel[1]);
	}
	printf("parent: waiting for child termination ...\n");
	wait(NULL);
	return 0;
}
