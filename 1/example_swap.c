#define _XOPEN_SOURCE /* Mac compatibility. */
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>

static ucontext_t uctx_main, uctx_func1, uctx_func2;

#define handle_error(msg) \
   do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define stack_size 1024 * 1024

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

static void *
allocate_stack_sig()
{
	void *stack = malloc(stack_size);
	stack_t ss;
	ss.ss_sp = stack;
	ss.ss_size = stack_size;
	ss.ss_flags = 0;
	sigaltstack(&ss, NULL);
	return stack;
}

static void *
allocate_stack_mmap()
{
	return mmap(NULL, stack_size, PROT_READ | PROT_WRITE | PROT_EXEC,
		    MAP_ANON | MAP_PRIVATE, -1, 0);
}

static void *
allocate_stack_mprot()
{
	void *stack = malloc(stack_size);
	mprotect(stack, stack_size, PROT_READ | PROT_WRITE | PROT_EXEC);
	return stack;
}

enum stack_type {
	STACK_MMAP,
	STACK_SIG,
	STACK_MPROT
};

static void *
allocate_stack(enum stack_type t)
{
	switch(t) {
	case STACK_MMAP:
		return allocate_stack_mmap();
	case STACK_SIG:
		return allocate_stack_sig();
	case STACK_MPROT:
		return allocate_stack_mprot();
	}
}

int
main(int argc, char *argv[])
{
	char *func1_stack = allocate_stack(STACK_SIG);
	char *func2_stack = allocate_stack(STACK_MPROT);

	if (getcontext(&uctx_func1) == -1)
		handle_error("getcontext");
	uctx_func1.uc_stack.ss_sp = func1_stack;
	uctx_func1.uc_stack.ss_size = stack_size;
	uctx_func1.uc_link = &uctx_main;
	makecontext(&uctx_func1, func1, 1, 1);

	if (getcontext(&uctx_func2) == -1)
		handle_error("getcontext");
	uctx_func2.uc_stack.ss_sp = func2_stack;
	uctx_func2.uc_stack.ss_size = stack_size;
	/* Successor context is f1(), unless argc > 1. */
	uctx_func2.uc_link = (argc > 1) ? NULL : &uctx_func1;
	makecontext(&uctx_func2, func1, 1, 2);

	printf("main: swapcontext(&uctx_main, &uctx_func2)\n");
	if (swapcontext(&uctx_main, &uctx_func2) == -1)
		handle_error("swapcontext");

	printf("main: exiting\n");
	return 0;
}
