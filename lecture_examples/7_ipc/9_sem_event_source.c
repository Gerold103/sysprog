#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

int
sem_get_or_create(key_t key)
{
	int semid = semget(key, 4, IPC_CREAT | IPC_EXCL | S_IRWXU | S_IRWXO);
	if (semid == -1) {
		if (errno != EEXIST) {
			printf("error = %s\n", strerror(errno));
			return -1;
		}
		semid = semget(key, 0, 0);
		if (semid == -1) {
			printf("error = %s\n", strerror(errno));
			return -1;
		}
		printf("Connected to sems %d\n", semid);
	} else {
		printf("Created sems with id %d\n", semid);
		unsigned short values[4] = {0, 0, 0, 1};
		semctl(semid, -1, SETALL, values);
	}
	return semid;
}

void
sem_inc(int semid, int myid)
{
	struct sembuf op;
	op.sem_flg = SEM_UNDO;
	op.sem_num = myid - 1;
	op.sem_op = 1;
	semop(semid, &op, 1);
}

int
msg_queue_get_or_create(key_t key)
{
	int queueid = msgget(key, IPC_CREAT | S_IRWXU | S_IRWXO);
	if (queueid == -1)
		printf("error = %s\n", strerror(errno));
	else
		printf("Created/connected to queue %d\n", queueid);
	return queueid;
}

int
pack_msg(char *msg, long type, const char *data, int data_size)
{
	char *pos = msg;
	memcpy(pos, &type, sizeof(type));
	pos += sizeof(type);
	memcpy(pos, &data_size, sizeof(data_size));
	pos += sizeof(data_size);
	memcpy(pos, data, data_size);
	pos += data_size;
	return pos - msg;
}

void
send_msg(int queueid, int myid, const char *data, int data_size)
{
	char msg[512];
	int size = pack_msg(msg, myid, data, data_size);
	msgsnd(queueid, msg, size, 0);
	printf("Sent %d bytes\n", size);
}

int
main(int argc, const char **argv)
{
	if (argc < 2) {
		printf("No id\n");
		return -1;
	}
	key_t key = ftok("./event_gen", 0);
	int semid = sem_get_or_create(key);
	if (semid == -1)
		return -1;
	int queueid = msg_queue_get_or_create(key);
	if (queueid == -1)
		return -1;

	int myid = atoi(argv[1]);
	
	char *line = NULL;
	size_t line_size = 0;
	while(1) {
		if (getline(&line, &line_size, stdin) <= 0)
			break;
		send_msg(queueid, myid, line, strlen(line));
		sem_inc(semid, myid);
	}
	if (myid == 1) {
		msgctl(queueid, IPC_RMID, NULL);
		semctl(semid, -1, IPC_RMID);
	}
	return 0;
}
