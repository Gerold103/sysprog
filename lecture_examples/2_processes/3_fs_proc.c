#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

int main()
{
	int fd_me = open("3_fs_proc.c", O_RDONLY);
	char *shared_mem = (char *) mmap(NULL, 100, PROT_READ,
					 MAP_FILE | MAP_SHARED, fd_me, 0);
	char buf[128];

	sprintf(buf, "/proc/%d/maps", (int) getpid());
	int fd = open(buf, O_RDONLY);
	printf("print %s\n", buf);
	if (fd == -1) {
		printf("exit %s\n", strerror(errno));
		exit(1);
	}
	int nbyte;
	while ((nbyte = read(fd, buf, sizeof(buf))) > 0)
		printf("%.*s", nbyte, buf);
	printf("\n");
	close(fd);
	munmap(shared_mem, 100);
	close(fd_me);
	return 0;
}
