#include <stdio.h>
#include <unistd.h>

int main()
{
	char buf[] = "write to 1\n";
	write(STDOUT_FILENO, buf, sizeof(buf));
	printf("stdout fileno = %d\n", STDOUT_FILENO);
	return 0;
}
