#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include "8_5_random_generator.h"

/**
 * Set this flag to 1 to stop reorderings, and to 0 to see them.
 */
#define USE_CPU_FENCE 0

/** Prevent compiler reordering. */
#define compiler_barrier() ({asm volatile("" ::: "memory");})
/** Prevent CPU reordering. */
#define memory_barrier() ({asm volatile("mfence" ::: "memory");})

#if USE_CPU_FENCE
	#define barrier() memory_barrier()
#else
	#define barrier() compiler_barrier()
#endif

static volatile bool start_t1 = false;
static volatile bool start_t2 = false;
static volatile bool end_t1 = false;
static volatile bool end_t2 = false;

static int X, Y;
static int r1, r2;

/**
 * The case - one thread sets X and reads Y. Other thread sets Y
 * and reads X. Logically it is impossible, that both threads see
 * the initial X and Y, but in fact it happens, without a proper
 * protection.
 */

void *
thread1_f(void *param)
{
	struct mersenne_twister twister;
	mersenne_twister_create(&twister, 1);
	for (;;)
	{
		/*
		 * Key moment - both threads should start the
		 * read-write pairs simultaneously. It increases
		 * reproducibility.
		 */
		while(! start_t1) { }
		start_t1 = false;
		compiler_barrier();
		/*
		 * The small random delay smooths the problem,
		 * that one thread can wakeup a bit earlier than
		 * another, and will perform its transaction too
		 * early.
		 */
		while (mersenne_twister_generate(&twister) % 8 != 0) {}
		compiler_barrier();

		/** ----- The transaction ----- */
		X = 1;
		barrier();
		r1 = Y;
		/** --------------------------- */

		compiler_barrier();
		end_t1 = true;
	}
	return NULL;
};

void *
thread2_f(void *param)
{
	struct mersenne_twister twister;
	mersenne_twister_create(&twister, 2);
	for (;;)
	{
		while(! start_t2) { }
		start_t2 = false;
		compiler_barrier();

		while (mersenne_twister_generate(&twister) % 8 != 0) {}
		compiler_barrier();

		/** ----- The transaction ----- */
		Y = 1;
		barrier();
		r2 = X;
		/** --------------------------- */

		compiler_barrier();
		end_t2 = true;
	}
	return NULL;
};

int
main()
{
	pthread_t thread1, thread2;
	pthread_create(&thread1, NULL, thread1_f, NULL);
	pthread_create(&thread2, NULL, thread2_f, NULL);

	int detected = 0;
	for (int iterations = 1; ; iterations++)
	{
		X = 0;
		Y = 0;
		start_t1 = true;
		start_t2 = true;
		compiler_barrier();

		while(! end_t1 || ! end_t2) { }
		compiler_barrier();
		end_t1 = false;
		end_t2 = false;

		if (r1 == 0 && r2 == 0)
		{
			detected++;
			printf("%d reorders detected after %d iterations\n",
			       detected, iterations);
		}
	}
	return 0;
}
