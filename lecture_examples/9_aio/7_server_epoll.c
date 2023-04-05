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
#include <poll.h>
#include <sys/epoll.h>
#include <sys/time.h>

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
	int ep = epoll_create(1);
	if (ep == -1) {
		printf("error = %s\n", strerror(errno));
		close(server);
		return -1;
	}
	struct epoll_event new_ev;
	new_ev.data.fd = server;
	new_ev.events = EPOLLIN;
	if (epoll_ctl(ep, EPOLL_CTL_ADD, server, &new_ev) == -1) {
		printf("error = %s\n", strerror(errno));
		close(server);
		return -1;
	}
	while(1) {
		int nfds = epoll_wait(ep, &new_ev, 1, 2000);
		if (nfds == 0) {
			printf("Timeout\n");
			continue;
		}
		if (nfds == -1) {
			printf("error = %s\n", strerror(errno));
			break;
		}
		if (new_ev.data.fd == server) {
			int client_sock = accept(server, NULL, NULL);
			if (client_sock == -1) {
				printf("error = %s\n", strerror(errno));
				break;
			}
			printf("New client\n");
			new_ev.data.fd = client_sock;
			new_ev.events = EPOLLIN;
			if (epoll_ctl(ep, EPOLL_CTL_ADD, client_sock,
				      &new_ev) == -1) {
				printf("error = %s\n", strerror(errno));
				break;
			}
		} else {
			printf("Interact with fd %d\n", (int)new_ev.data.fd);
			int rc = interact(new_ev.data.fd);
			if (rc == -1) {
				printf("error = %s\n", strerror(errno));
				if (errno != EWOULDBLOCK && errno != EAGAIN)
					break;
			} else if (rc == 0) {
				printf("Client disconnected\n");
				epoll_ctl(ep, EPOLL_CTL_DEL, new_ev.data.fd,
					  NULL);
				close(new_ev.data.fd);
			}
		}
	}
	close(ep);
	close(server);
	return 0;
}
