#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

int
main()
{
	mode_t mode = S_IRUSR | S_IXUSR;
	int fd = open("tmp.txt", O_CREAT | O_RDWR, mode);
	if (fd == -1)
		printf("error = %s\n", strerror(errno));
	else
		close(fd);
	return 0;
}
