#include <sys/ipc.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

void
try_key(const char *path, int id)
{
	key_t key = ftok(path, id);
	printf("key(\"%s\", %d) = ", path, id);
	if (key == -1)
		printf("%s\n", strerror(errno));
	else
		printf("%d\n", (int) key);
}

int main(int argc, const char **argv)
{
	try_key("/not/exising/path", 100);
	try_key(argv[0], 100);
	try_key(argv[0], 101);
	try_key(argv[0], 101 + 1024);
	return 0;
}
