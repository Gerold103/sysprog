#include <signal.h>
#include <setjmp.h>
#include <stdio.h>

static jmp_buf jmp;

void
process_signal(int code)
{
	printf("Process SIGSEGV, code = %d, SIGSEGV = %d\n", code, SIGSEGV);
	longjmp(jmp, 1);
}

int
main()
{
	char *p = NULL;
	signal(SIGSEGV, process_signal);
	printf("Before SIGSEGV\n");
	if (setjmp(jmp) == 0)
		*p = 100;
	printf("After SIGSEGV\n");
	return 0;
}
