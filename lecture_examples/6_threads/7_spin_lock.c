#include <sched.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

struct complex_thing {
	bool lock;
	char string[128];
};

volatile struct complex_thing thing;

volatile bool start = false;

void *
thread_f(void *arg)
{
	int id = (int) arg;
	while (! start) {};
	if (__sync_bool_compare_and_swap(&thing.lock, 0, 1)) {
		for (int i = 0; i < sizeof(thing.string) - 1; ++i)
			thing.string[i] = 'a' + id;
		__sync_bool_compare_and_swap(&thing.lock, 1, 0);
	}
	return NULL;
}

int
main()
{
	pthread_t tid[6];
	for (int i = 0; i < 6; ++i)
		pthread_create(&tid[i], NULL, thread_f, (void *) i);
	start = true;
	for (int i = 0; i < 6; ++i)
		pthread_join(tid[i], NULL);
	printf("%s\n", thing.string);
	return 0;
}
