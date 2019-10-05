#include <sched.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>

/**
 * Swap to 0 to look at performance difference between ACQ+REL vs
 * full barrier.
 */
#define USE_ACQUIRE_RELEASE 1

#if USE_ACQUIRE_RELEASE

static void
spin_lock(volatile bool *lock)
{
	while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE))
	{}
}

static void
spin_unlock(volatile bool *lock)
{
	__atomic_clear(lock, __ATOMIC_RELEASE);
}

#else

static void
spin_lock(volatile bool *lock)
{
	while (! __sync_bool_compare_and_swap(lock, 0, 1))
	{}
}

static void
spin_unlock(volatile bool *lock)
{
	__sync_bool_compare_and_swap(lock, 1, 0);
}

#endif

static volatile bool start = false;
static uint64_t counter = 0;
static volatile bool lock = false;

void *
thread_f(void *arg)
{
	while (! start) {};
	for (int i = 0; i < 10000000; ++i) {
		spin_lock(&lock);
		++counter;
		spin_unlock(&lock);
	}
	return NULL;
}

int
main()
{
	pthread_t tid[6];
	for (int i = 0; i < 6; ++i)
		pthread_create(&tid[i], NULL, thread_f, (void *) i);
	clock_t ts1 = clock();
	start = true;
	for (int i = 0; i < 6; ++i)
		pthread_join(tid[i], NULL);
	clock_t ts2 = clock();
	printf("counter = %llu, time = %lf\n", counter,
	       ((double) (ts2 - ts1)) / CLOCKS_PER_SEC);
	return 0;
}
