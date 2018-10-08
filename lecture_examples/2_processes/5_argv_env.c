#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv, char **env)
{
	char *path = getenv("PATH");
	printf("env: %p\n", path);
	printf("argv: %p\n", argv);
	int a;
	printf("stack: %p\n", &a);
	void *m = malloc(100);
	printf("heap: %p\n", m);
	free(m);

	int i = 0;
	while (env[i] != NULL)
		printf("%s\n", env[i++]);
	return 0;
}
