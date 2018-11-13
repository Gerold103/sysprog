#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

void
initiator_f(int sock, struct sockaddr_un *next_addr)
{
	int number;
	while(scanf("%d", &number) > 0) {
		if (sendto(sock, &number, sizeof(number), 0,
			   (struct sockaddr *) next_addr,
			   sizeof(*next_addr)) == -1) {
			printf("error = %s\n", strerror(errno));
			continue;
		}
		printf("Sent %d\n", number);
		int buffer = 0;
		ssize_t size = recv(sock, &buffer, sizeof(buffer), 0);
		if (size == -1)
			printf("error = %s\n", strerror(errno));
		else
			printf("Received %d\n", buffer);
	}
}

void
worker_f(int sock, struct sockaddr_un *next_addr)
{
	while(1) {
		int buffer = 0;
		ssize_t size = recv(sock, &buffer, sizeof(buffer), 0);
		if (size == -1) {
			printf("error = %s\n", strerror(errno));
			continue;
		}
		printf("Received %d\n", buffer);
		buffer++;
		if (sendto(sock, &buffer, sizeof(buffer), 0,
			  (struct sockaddr *) next_addr,
			  sizeof(*next_addr)) == -1)
			printf("error = %s\n", strerror(errno));
		else
			printf("Sent %d\n", buffer);
	}
}

int
main(int argc, const char **argv)
{
	if (argc < 2) {
		printf("No id\n");
		return -1;
	}
	int myid = atoi(argv[1]);
	int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock == -1) {
		printf("error = %s\n", strerror(errno));
		return -1;
	}
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	sprintf(addr.sun_path, "sock%d", myid);
	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
		printf("error = %s\n", strerror(errno));
		return -1;
	}

	struct sockaddr_un next_addr;
	next_addr.sun_family = AF_UNIX;
	sprintf(next_addr.sun_path, "sock%d", (myid + 1) % 3);

	ssize_t size;
	int buffer;
	if (myid == 0)
		initiator_f(sock, &next_addr);
	else
		worker_f(sock, &next_addr);
	return 0;
}
