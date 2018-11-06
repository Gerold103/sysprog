#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>

int
futex_wait(int *futex, int val)
{
	return syscall(SYS_futex, futex, FUTEX_WAIT, val,
		       NULL, NULL, 0);
}

int
futex_wake(int *futex)
{
	return syscall(SYS_futex, futex, FUTEX_WAKE, 1,
		       NULL, NULL, 0);
}

void
futex_lock(int *futex)
{
	while (__sync_val_compare_and_swap(futex, 0, 1) != 0)
		futex_wait(futex, 1);
}

void
futex_unlock(int *futex)
{
	__sync_bool_compare_and_swap(futex, 1, 0);
	futex_wake(futex);
}

volatile int counter = 0;
volatile bool start = false;
volatile int futex = 0;

void *
thread_f(void *futex)
{
	while (! start) {};

	for (int i = 0; i < 100000; ++i) {
		futex_lock((int *) futex);
		counter = counter + 1;
		futex_unlock((int *) futex);
	}
	return NULL;
}

int
main()
{
	pthread_t tid[6];
	int futex = 0;
	for (int i = 0; i < 6; ++i)
		pthread_create(&tid[i], NULL, thread_f, (void *) &futex);
	start = true;
	for (int i = 0; i < 6; ++i)
		pthread_join(tid[i], NULL);
	printf("%d\n", counter);
	return 0;
}
