#include <stdio.h>
#include <string.h>

int uninitialized;

const char *str = "const char *str";
const char str2[] = "const char str2[]";

void
another_function(void)
{
	char array[128];
	memset(array, 0, sizeof(array));
	printf("called another function, stack is %p\n", array);
}

void
test_stack(void)
{
	int a;
	printf("stack top in test_stack: %p\n", &a);
	const char *str3 = "const char *str3";
	const char str4[] = "const char str4[]";
	char str5[] = "char str5[]";
	char b = 'x';
	char c = 'x';
	char d = 'x';
	int e = 32;
	int f = 64;
	int g = 128;
	printf("a = %d\n", a);
	a = 10;
}

int
main(void)
{
	int a = 20;
	printf("stack top in main: %p\n", &a);
	test_stack();
	test_stack();
	another_function();
	test_stack();
	return 0;
}
