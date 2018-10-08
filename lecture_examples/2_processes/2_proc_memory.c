#include <stdio.h>

int uninitialized;

const char *str = "const char *str";
const char str2[] = "const char str2[]";

void
test_stack()
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

int main()
{
	int a = 20;
	printf("stack top in main: %p\n", &a);
	test_stack();
	test_stack();
	return 0;
}
