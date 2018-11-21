#include <stdio.h>
#include <sys/file.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int
main()
{
	int fd = open("tmp.txt", O_CREAT | O_RDWR, S_IRWXU);
	if (fd == -1) {
		printf("error = %s\n", strerror(errno));
		return -1;
	}
	char cmd[100];
	while (scanf("%s", cmd) > 0) {
		int rc;
		if (strcmp(cmd, "excl") == 0) {
			rc = flock(fd, LOCK_EX);
		} else if (strcmp(cmd, "exclnb") == 0) {
			rc = flock(fd, LOCK_EX | LOCK_NB);
		} else if (strcmp(cmd, "shared") == 0) {
			rc = flock(fd, LOCK_SH);
		} else if (strcmp(cmd, "sharednb") == 0) {
			rc = flock(fd, LOCK_SH | LOCK_NB);
		} else if (strcmp(cmd, "unlock") == 0) {
			rc = flock(fd, LOCK_UN);
		} else if (strcmp(cmd, "write") == 0) {
			rc = write(fd, "data", 4);
		} else {
			printf("unknown command\n");
			continue;
		}
		if (rc == -1)
			printf("error = %s\n", strerror(errno));
		else
			printf("ok\n");
	}
	close(fd);
	return 0;
}
