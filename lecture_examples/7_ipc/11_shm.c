#include <assert.h>
#include <stdint.h>
#include <sys/wait.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int
main()
{
	key_t key = ftok("./a.out", 0);
	int id = shmget(key, 1024, IPC_CREAT | S_IRWXU | S_IRWXO);
	printf("Created mem with id %d\n", id);
	char *mem = shmat(id, NULL, 0);
	int align = __alignof__(pthread_mutex_t);
	pthread_mutex_t *mutex = (pthread_mutex_t *)
		(mem + align - (uint64_t)mem % align);
	volatile char *data = (char *) mutex + sizeof(*mutex);
	*data = 0;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
	pthread_mutex_init(mutex, &attr);
	pthread_mutexattr_destroy(&attr);
	printf("Created mutex between processes\n");

	if (fork() == 0) {
		printf("Child holds a lock\n");
		pthread_mutex_lock(mutex);
		*data = 1;
		printf("And the child dies\n");
		exit(1);
	}
	printf("Parent waits for 1 in shmem\n");
	wait(NULL);
	assert(*data == 1);
	printf("Parent tries to lock\n");
	if (pthread_mutex_lock(mutex) == EOWNERDEAD) {
		printf("Owner is dead, restore\n");
		pthread_mutex_consistent(mutex);
	}
	printf("Destroy mutex\n");
	pthread_mutex_unlock(mutex);
	pthread_mutex_destroy(mutex);

	printf("Free shmem\n");
	shmdt(mem);
	shmctl(id, IPC_RMID, NULL);
	return 0;
}
