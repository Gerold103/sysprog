#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

volatile bool finish = false;
volatile bool ptr_is_set = false;
volatile int *a_ptr = NULL;

void *
thread_f(void *arg)
{
	int a = 100;
	printf("child stack top = %p\n", &a);
	a_ptr = &a;
	ptr_is_set = true;
	while (! finish) {}
	return NULL;
}

int
main()
{
	pthread_t t;
	printf("main stack top = %p\n", &t);
	pthread_create(&t, NULL, thread_f, NULL);
	while (! ptr_is_set) {}
	printf("foreign a = %d\n", *a_ptr);
	finish = true;
	pthread_join(t, NULL);
	return 0;
}
