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
#include <sys/event.h>
#include <sys/time.h>

struct peer {
	int fd;
	struct peer *next;
	struct peer *prev;
};

int
interact(struct peer *p)
{
	int buffer = 0;
	ssize_t size = recv(p->fd, &buffer, sizeof(buffer), 0);
	if (size <= 0)
		return (int) size;
	printf("Received %d\n", buffer);
	buffer++;
	size = send(p->fd, &buffer, sizeof(buffer), 0);
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
	int kq = kqueue();
	if (kq == -1) {
		printf("error = %s\n", strerror(errno));
		close(server);
		return -1;
	}
	struct kevent new_ev;
	EV_SET(&new_ev, server, EVFILT_READ, EV_ADD, 0, 0, 0);
	if (kevent(kq, &new_ev, 1, 0, 0, NULL) == -1) {
		printf("error = %s\n", strerror(errno));
		close(server);
		return -1;
	}
	struct peer *peers = NULL;
	struct timespec timeout;
	timeout.tv_sec = 2;
	timeout.tv_nsec = 0;
	while(1) {
		int nfds = kevent(kq, NULL, 0, &new_ev, 1, &timeout);
		if (nfds == 0) {
			printf("Timeout\n");
			continue;
		}
		if (nfds == -1) {
			printf("error = %s\n", strerror(errno));
			break;
		}
		if (new_ev.udata == NULL) {
			int peer_sock = accept(server, NULL, NULL);
			if (peer_sock == -1) {
				printf("error = %s\n", strerror(errno));
				break;
			}
			printf("New client\n");
			struct peer *p = malloc(sizeof(*p));
			EV_SET(&new_ev, peer_sock, EVFILT_READ, EV_ADD,
			       0, 0, p);
			if (kevent(kq, &new_ev, 1, 0, 0, NULL) == -1) {
				printf("error = %s\n", strerror(errno));
				close(peer_sock);
				free(p);
				break;
			}
			p->fd = peer_sock;
			p->next = peers;
			p->prev = NULL;
			if (peers != NULL)
				peers->prev = p;
			peers = p;
			continue;
		}
		struct peer *p = new_ev.udata;
		printf("Interact with fd %d\n", p->fd);
		int rc = interact(p);
		if (rc == -1) {
			printf("error = %s\n", strerror(errno));
			if (errno != EWOULDBLOCK && errno != EAGAIN)
				break;
			continue;
		}
		if ((new_ev.flags & EV_EOF) == 0)
			continue;

		printf("Client disconnected\n");
		if (p->prev != NULL)
			p->prev->next = p->next;
		if (p->next != NULL)
			p->next->prev = p->prev;
		if (p == peers)
			peers = p->next;
		close(p->fd);
		free(p);
	}
	while (peers != NULL) {
		struct peer *next = peers->next;
		close(peers->fd);
		free(peers);
		peers = next;
	}
	close(kq);
	close(server);
	return 0;
}
