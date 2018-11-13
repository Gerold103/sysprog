#define _GNU_SOURCE
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, const char **argv)
{
	key_t key = ftok(argv[0], 0);
	int queue_id = msgget(key, IPC_CREAT | S_IRWXU | S_IRWXO);
	printf("key = %d, queue_id = %d\n", (int) key, queue_id);

	struct msqid_ds queue_stat;
	msgctl(queue_id, IPC_STAT, &queue_stat);
	printf("message count = %d\n", (int) queue_stat.msg_qnum);
	printf("max queue bytes = %d\n", (int) queue_stat.msg_qbytes);

	int rc = msgget(key, IPC_CREAT | IPC_EXCL);
	if (rc == -1)
		printf("second msgget returned '%s'\n", strerror(errno));

	msgctl(queue_id, IPC_RMID, NULL);

#ifdef IPC_INFO
	printf("\nSystem wide settings:\n");
	struct msginfo info;
	msgctl(0, IPC_INFO, (struct msqid_ds *) &info);

	printf("max message size = %d\n", info.msgmax);
	printf("max queue bytes = %d\n", info.msgmnb);
	printf("max number of message queues = %d\n", info.msgmni);
#endif
	return 0;
}
