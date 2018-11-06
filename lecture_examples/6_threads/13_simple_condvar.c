#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <sched.h>

struct condvar {
	pthread_mutex_t event_lock;
	bool is_event_set;
};

void
condvar_signal(struct condvar *condvar)
{
	printf("condvar signaled\n");
	condvar->is_event_set = true;
	pthread_mutex_unlock(&condvar->event_lock);

	pthread_mutex_lock(&condvar->event_lock);
	condvar->is_event_set = false;
	printf("condvar is locked\n");
}

void
condvar_wait(struct condvar *condvar)
{
	for (;;) {
		pthread_mutex_lock(&condvar->event_lock);
		if (condvar->is_event_set) {
			condvar->is_event_set = false;
			pthread_mutex_unlock(&condvar->event_lock);
			return;
		}
		pthread_mutex_unlock(&condvar->event_lock);
	}
}

void
condvar_create(struct condvar *condvar)
{
	pthread_mutex_init(&condvar->event_lock, NULL);
	pthread_mutex_lock(&condvar->event_lock);
	condvar->is_event_set = false;
}

struct condvar condvar;

void *
thread_f(void *arg)
{
	int id = (int) arg;
	for (;;) {
		condvar_wait(&condvar);
		printf("%d processed the event\n", id);
	}
}

int
main()
{
	condvar_create(&condvar);
	const int thread_count = 10;
	pthread_t tid[thread_count];
	for (int i = 0; i < thread_count; ++i)
		pthread_create(&tid[i], NULL, thread_f, (void *) i);
	while (getchar() != EOF)
		condvar_signal(&condvar);
	for (int i = 0; i < thread_count; ++i)
		pthread_join(tid[i], NULL);
	return 0;
}
