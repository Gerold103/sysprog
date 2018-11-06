#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

volatile int ready_to_print = 0;
volatile bool print = false;

pthread_key_t key;

void *
thread_f(void *arg)
{
	pthread_setspecific(key, arg);
	__sync_fetch_and_add(&ready_to_print, 1);
	while (! print) {}
	int tmp = (int) pthread_getspecific(key);
	printf("value = %d\n", tmp);
	return NULL;
}

int
main()
{
	pthread_t t1, t2;
	pthread_key_create(&key, NULL);
	pthread_setspecific(key, (const void *) 0);

	pthread_create(&t1, NULL, thread_f, (void *) 1);
	pthread_create(&t2, NULL, thread_f, (void *) 2);
	while (__sync_fetch_and_add(&ready_to_print, 0) != 2) {}
	print = true;
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	int tmp = (int) pthread_getspecific(key);
	printf("main value = %d\n", tmp);
	return 0;
}
