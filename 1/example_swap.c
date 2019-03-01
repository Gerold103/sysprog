#define _XOPEN_SOURCE /* Mac compatibility. */
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>

/**
 * You can compile and run this example using the commands:
 *
 * $> gcc example_swap.c
 * $> ./a.out
 */

/**
 * Here you have 3 contexts - one for each coroutine. First
 * belongs to main coroutine - the one, who starts all others.
 * Next two are worker coroutines - they do real work. In your
 * code you will have more than 2.
 */
static ucontext_t uctx_main, uctx_func1, uctx_func2;

#define handle_error(msg) \
   do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define stack_size 1024 * 1024

/**
 * Coroutine body. This code is executed by all the corutines at
 * the same time and with different stacks. So you can store any
 * local variables and arguments right here. Here you implement
 * your solution.
 */
static void
my_coroutine(int id)
{
	printf("func%d: started\n", id);
	if (id == 1) {
	        printf("coroutine1: swapcontext(&uctx_func1, &uctx_func2)\n");
		if (swapcontext(&uctx_func1, &uctx_func2) == -1)
	        	handle_error("swapcontext");
	} else {
		printf("coroutine2: swapcontext(&uctx_func2, &uctx_func1)\n");
		if (swapcontext(&uctx_func2, &uctx_func1) == -1)
			handle_error("swapcontext");
	}
	printf("func%d: returning\n", id);
}

/**
 * Below you can see 3 different ways of how to allocate stack.
 * You can choose any. All of them do in fact the same.
 */

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

/**
 * Use this wrapper to choose your favourite way of stack
 * allocation.
 */
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
	/* First of all, create a stack for each coroutine. */
	char *func1_stack = allocate_stack(STACK_SIG);
	char *func2_stack = allocate_stack(STACK_MPROT);

	/*
	 * Below is just initialization of coroutine structures.
	 * They are not started yet. Just created.
	 */
	if (getcontext(&uctx_func1) == -1)
		handle_error("getcontext");
	/*
	 * Here you specify a stack, allocated earlier. Unique for
	 * each coroutine.
	 */
	uctx_func1.uc_stack.ss_sp = func1_stack;
	uctx_func1.uc_stack.ss_size = stack_size;
	/*
	 * Important - here you specify, to which context to
	 * switch after this coroutine is finished. The code below
	 * says, that when 'uctx_func1' is finished, it should
	 * switch to 'uctx_main'.
	 */
	uctx_func1.uc_link = &uctx_main;
	makecontext(&uctx_func1, my_coroutine, 1, 1);

	if (getcontext(&uctx_func2) == -1)
		handle_error("getcontext");
	uctx_func2.uc_stack.ss_sp = func2_stack;
	uctx_func2.uc_stack.ss_size = stack_size;
	/* Successor context is f1(), unless argc > 1. */
	uctx_func2.uc_link = (argc > 1) ? NULL : &uctx_func1;
	makecontext(&uctx_func2, my_coroutine, 1, 2);

	/*
	 * And here it starts. The first coroutine to start is
	 * 'uctx_func2'.
	 */
	printf("main: swapcontext(&uctx_main, &uctx_func2)\n");
	if (swapcontext(&uctx_main, &uctx_func2) == -1)
		handle_error("swapcontext");

	printf("main: exiting\n");
	return 0;
}
