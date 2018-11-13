#include <sys/socket.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>

#define MAX_MSG_SIZE 1024 * 1024

void
worker(int sock)
{
	char buffer[MAX_MSG_SIZE];
	ssize_t size;
	while ((size = read(sock, buffer, sizeof(buffer))) > 0)
		printf("Client received %d\n", (int) size);
	close(sock);
}

int
main(int argc, const char **argv)
{
	if (argc < 2) {
		printf("No socket type\n");
		return -1;
	}
	int socket_type;
	if (strcmp(argv[1], "stream"))
		socket_type = SOCK_STREAM;
	else
		socket_type = SOCK_DGRAM;
	int sockets[2];
	socketpair(AF_UNIX, socket_type, 0, sockets);
	if (fork() == 0) {
		close(sockets[0]);
		worker(sockets[1]);
		return 0;
	}
	close(sockets[1]);
	char buffer[MAX_MSG_SIZE];
	int size;
	while (scanf("%d", &size) > 0) {
		printf("Server sent %d\n", size);
		write(sockets[0], buffer, size);
	}
	close(sockets[0]);
	return 0;
}
