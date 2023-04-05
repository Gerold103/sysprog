#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>

int
interact(int client_sock)
{
	int buffer = 0;
	ssize_t size = recv(client_sock, &buffer, sizeof(buffer), 0);
	if (size <= 0)
		return (int) size;
	printf("Received %d\n", buffer);
	buffer++;
	size = send(client_sock, &buffer, sizeof(buffer), 0);
	if (size > 0)
		printf("Sent %d\n", buffer);
	return (int) size;
}

int
fill_fdset(fd_set *set, int *clients, int client_count, int server)
{
	int max_fd = server;
	FD_ZERO(set);
	FD_SET(server, set);
	for (int i = 0; i < client_count; ++i) {
		FD_SET(clients[i], set);
		if (clients[i] > max_fd)
			max_fd = clients[i];
	}
	return max_fd;
}

void
remove_client(int **clients, int *client_count, int i)
{
	--*client_count;
	int *new_clients = malloc(*client_count * sizeof(int));
	memcpy(new_clients, *clients, i * sizeof(int));
	memcpy(new_clients + i, *clients + i + 1,
	       (*client_count - i) * sizeof(int));
	free(*clients);
	*clients = new_clients;
}

int
main(int argc, const char **argv)
{
	int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server == -1) {
		printf("error = %s\n", strerror(errno));
		return -1;
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	inet_aton("127.0.0.1", &addr.sin_addr);

	if (bind(server, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
		printf("bind error = %s\n", strerror(errno));
		return -1;
	}
	if (listen(server, 128) == -1) {
		printf("listen error = %s\n", strerror(errno));
		return -1;
	}
	int *clients = NULL;
	int client_count = 0;
	fd_set readset;
	while(1) {
		int max_fd = fill_fdset(&readset, clients, client_count,
					server);
		struct timeval timeval;
		timeval.tv_sec = 2;
		timeval.tv_usec = 0;
		int nfds = select(max_fd + 1, &readset, NULL, NULL, &timeval);
		if (nfds == 0) {
			printf("Timeout\n");
			continue;
		}
		if (nfds == -1) {
			printf("error = %s\n", strerror(errno));
			break;
		}
		if (FD_ISSET(server, &readset)) {
			int client_sock = accept(server, NULL, NULL);
			if (client_sock == -1) {
				printf("error = %s\n", strerror(errno));
				break;
			}
			printf("New client\n");
			client_count++;
			clients = realloc(clients, client_count * sizeof(int));
			clients[client_count - 1] = client_sock;
			nfds--;
		}
		for (int i = 0; i < client_count && nfds > 0; ++i) {
			if (! FD_ISSET(clients[i], &readset))
				continue;
			nfds--;
			printf("Interact with fd %d\n", clients[i]);
			int rc = interact(clients[i]);
			if (rc == -1) {
				printf("error = %s\n", strerror(errno));
				if (errno != EWOULDBLOCK && errno != EAGAIN)
					break;
			} else if (rc == 0) {
				printf("Client disconnected\n");
				remove_client(&clients, &client_count, i);
			}
		}
	}
	close(server);
	for (int i = 0; i < client_count; ++i)
		close(clients[i]);
	free(clients);
	return 0;
}
