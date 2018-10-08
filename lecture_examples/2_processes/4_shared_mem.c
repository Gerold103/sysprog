#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>

int main()
{
	char *shared_mem = (char *) mmap(NULL, 100, PROT_READ | PROT_WRITE,
					 MAP_ANON | MAP_SHARED, -1, 0);
	char *private_mem = (char *) malloc(100);
	shared_mem[0] = 55;
	private_mem[0] = 55;
	if (fork() == 0) {
		shared_mem[0] = 56;
		private_mem[0] = 56;
		goto exit;
	}
	wait(NULL);
	printf("shared: %d, private: %d\n", (int) shared_mem[0],
	       (int) private_mem[0]);
exit:
	munmap(shared_mem, 100);
	free(private_mem);
	return 0;
}
