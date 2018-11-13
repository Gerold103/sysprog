#include <pthread.h>

struct semaphore {
	int counter;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

static inline void
semaphore_get(struct semaphore *sem)
{
	pthread_mutex_lock(&sem->mutex);
	while (sem->counter == 0)
		pthread_cond_wait(&sem->cond, &sem->mutex);
	sem->counter--;
	pthread_mutex_unlock(&sem->mutex);
}

static inline void
semaphore_put(struct semaphore *sem)
{
	pthread_mutex_lock(&sem->mutex);
	sem->counter++;
	pthread_cond_signal(&sem->cond);
	pthread_mutex_unlock(&sem->mutex);
}
