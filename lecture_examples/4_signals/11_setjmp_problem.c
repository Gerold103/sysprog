#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>

static volatile jmp_buf buf;
static volatile bool stop = false;
static volatile int total = 0;

static void
testf2(void)
{
	setjmp(buf);
	if (++total > 10) {
		printf("Bad exit\n");
		exit(-1);
	}
}

static void
testf(void)
{
	testf2();
}

int
main(void)
{
	testf();
	if (! stop) {
		stop = true;
		printf("Аfter first call\n");
		longjmp(buf, 1);
	}
	return 0;
}
