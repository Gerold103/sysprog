#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define WORKER_COUNT 10

sem_t *sem;

void *
worker_f(void *arg)
{
	int id = (int) arg;
	while (1) {
		sem_wait(sem);
		printf("Thread %d got the sem\n", id);
		sleep(3);
		sem_post(sem);
		sleep(1);
	}
}

int
main()
{
retry:;
	pthread_t workers[WORKER_COUNT];
	sem = sem_open("/my_sem", O_CREAT | O_EXCL, O_RDWR, 5);
	if (sem == SEM_FAILED) {
		if (errno == EEXIST) {
			sem_unlink("/my_sem");
			goto retry;
		}
		printf("error = %s\n", strerror(errno));
		return -1;
	}
	for (int i = 0; i < WORKER_COUNT; ++i)
		pthread_create(&workers[i], NULL, worker_f, (void *) i);
	getchar();
	sem_close(sem);
	sem_unlink("/my_sem");
	return 0;
}
