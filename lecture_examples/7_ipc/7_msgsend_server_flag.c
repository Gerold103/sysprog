#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <sched.h>
#include <fcntl.h>

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
recv_msg(int queue_id, long type)
{
	char msg[512];
	ssize_t recv;
	while(1) {
		recv = msgrcv(queue_id, msg, sizeof(msg), type, IPC_NOWAIT);
		if (recv != -1)
			break;
		if (errno != ENOMSG) {
			printf("error = %s\n", strerror(errno));
			return;
		}
	}
	printf("received %d\n", (int) recv);
	const char *data;
	int data_size;
	unpack_msg(msg, recv, &type, &data, &data_size);
	printf("type = %ld, size = %d, str = %s\n", type, data_size, data);
}

int main(int argc, const char **argv)
{
	key_t key = ftok("./server", 0);
	int queue_id = msgget(key, IPC_CREAT | S_IRWXU | S_IRWXO);
	printf("connected to queue %d\n", queue_id);

	recv_msg(queue_id, 0);

	recv_msg(queue_id, 60);

	recv_msg(queue_id, -200);

	msgctl(queue_id, IPC_RMID, NULL);
	return 0;
}
