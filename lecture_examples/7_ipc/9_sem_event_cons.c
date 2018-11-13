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
unpack_msg(const char *msg, int size, long *type, const char **data,
	   int *data_size)
{
	const char *pos = msg;
	memcpy(type, pos, sizeof(*type));
	pos += sizeof(type);
	memcpy(data_size, pos, sizeof(*data_size));
	pos += sizeof(*data_size);
	*data = pos;
	pos += *data_size;
	return pos - msg;
}

void
recv_msg(int queueid, long type)
{
	char msg[512];
	ssize_t recv = msgrcv(queueid, msg, sizeof(msg), type, 0);
	printf("Received %d\n", (int) recv);
	const char *data;
	int data_size;
	unpack_msg(msg, recv, &type, &data, &data_size);
	printf("Type = %ld, size = %d, str = %s\n", type, data_size, data);
}

void
read_data(int queueid)
{
	for (int i = 0; i < 3; ++i)
		recv_msg(queueid, i + 1);
}

void
unlock_events(int semid)
{
	struct sembuf op;
	op.sem_flg = SEM_UNDO;
	op.sem_num = 3;
	op.sem_op = 1;
	semop(semid, &op, 1);
}

void
lock_events(int semid)
{
	struct sembuf op[4];
	for (int i = 0; i < 4; ++i) {
		op[i].sem_flg = SEM_UNDO;
		op[i].sem_num = i;
		op[i].sem_op = -1;
	}
	semop(semid, op, 4);
}

int
main()
{
	key_t key = ftok("./event_gen", 0);
	int semid = semget(key, 4, 0);
	int queueid = msgget(key, 0);

	while(1) {
		lock_events(semid);
		read_data(queueid);
		unlock_events(semid);
	}
	return 0;
}
