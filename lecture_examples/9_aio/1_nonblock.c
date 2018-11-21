#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void
make_fd_nonblocking(int fd)
{
	int old_flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, old_flags | O_NONBLOCK);
}

int
main(int argc, const char **argv)
{
	make_fd_nonblocking(STDIN_FILENO);
	int value = 0;
	int rc = scanf("%d", &value);
	printf("scanf rc = %d\n", rc);
	printf("scanf error = %s\n", strerror(errno));
	rc = read(STDIN_FILENO, &value, sizeof(value));
	printf("read rc = %d\n", rc);
	printf("read error = %s\n", strerror(errno));

	while (1) {
		rc = scanf("%d", &value);
		if (rc > 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
			break;
	}
	printf("value = %d\n", value);
	return 0;
}
