#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>

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
send_msg(int queue_id, long type, const char *data, int data_size)
{
	char msg[512];
	int size = pack_msg(msg, type, data, data_size);
	msgsnd(queue_id, msg, size, 0);
	printf("sent %d bytes\n", size);
}

int
main(int argc, const char **argv)
{
	key_t key = ftok("./server", 0);
	int queue_id = msgget(key, 0);
	printf("connected to queue %d\n", queue_id);

	const char *text = "hello, world";
	send_msg(queue_id, 100, text, strlen(text) + 1);

	text = "hello, world of type 50";
	send_msg(queue_id, 50, text, strlen(text) + 1);

	text = "hello, world of type 60";
	send_msg(queue_id, 60, text, strlen(text) + 1);
	return 0;
}
