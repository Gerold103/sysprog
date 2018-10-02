#define _XOPEN_SOURCE /* Mac compatibility. */
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>

static ucontext_t uctx_main, uctx_func1, uctx_func2;

#define handle_error(msg) \
   do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void
func1(int id)
{
	printf("func%d: started\n", id);
	if (id == 1) {
	        printf("func1: swapcontext(&uctx_func1, &uctx_func2)\n");
		if (swapcontext(&uctx_func1, &uctx_func2) == -1)
	        	handle_error("swapcontext");
	} else {
		printf("func2: swapcontext(&uctx_func2, &uctx_func1)\n");
		if (swapcontext(&uctx_func2, &uctx_func1) == -1)
			handle_error("swapcontext");
	}
	printf("func%d: returning\n", id);
}

int
main(int argc, char *argv[])
{
	char func1_stack[64384];
	char func2_stack[64384];

	if (getcontext(&uctx_func1) == -1)
		handle_error("getcontext");
	uctx_func1.uc_stack.ss_sp = func1_stack;
	uctx_func1.uc_stack.ss_size = sizeof(func1_stack);
	uctx_func1.uc_link = &uctx_main;
	makecontext(&uctx_func1, func1, 1, 1);

	if (getcontext(&uctx_func2) == -1)
		handle_error("getcontext");
	uctx_func2.uc_stack.ss_sp = func2_stack;
	uctx_func2.uc_stack.ss_size = sizeof(func2_stack);
	/* Successor context is f1(), unless argc > 1. */
	uctx_func2.uc_link = (argc > 1) ? NULL : &uctx_func1;
	makecontext(&uctx_func2, func1, 1, 2);

	printf("main: swapcontext(&uctx_main, &uctx_func2)\n");
	if (swapcontext(&uctx_main, &uctx_func2) == -1)
		handle_error("swapcontext");

	printf("main: exiting\n");
	return 0;
}
