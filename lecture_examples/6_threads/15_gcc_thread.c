#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static __thread int a = -1;

volatile int ready_to_print = 0;
volatile bool print = false;

void *
thread_f(void *arg)
{
	a = (int) arg;
	__sync_fetch_and_add(&ready_to_print, 1);
	while (! print) {}
	printf("a = %d\n", a);
	return NULL;
}

int
main()
{
	pthread_t t1, t2;
	a = 0;
	pthread_create(&t1, NULL, thread_f, (void *) 1);
	pthread_create(&t2, NULL, thread_f, (void *) 2);
	while (__sync_fetch_and_add(&ready_to_print, 0) != 2) {}
	print = true;
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	printf("main a = %d\n", a);
	return 0;
}
