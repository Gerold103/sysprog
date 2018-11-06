#include <sched.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

void *
thread_f(void *arg)
{
	*((bool *)arg) = true;
	return NULL;
}

int
main()
{
	const volatile bool is_finished = false;
	pthread_t tid;
	pthread_create(&tid, NULL, thread_f,
		       (void *) &is_finished);
	while (! is_finished)
		sched_yield();

	pthread_join(tid, NULL);
	return 0;
}
