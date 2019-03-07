#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>

int
main()
{
	struct addrinfo *addr, *iter;
	int rc = getaddrinfo("yandex.ru", NULL, NULL, &addr);
	if (rc != 0) {
		printf("Error = %s\n", gai_strerror(rc));
		return -1;
	}
	printf("Families: inet = %d, inet6 = %d\n", AF_INET, AF_INET6);
	printf("Socket types: dgram = %d, stream = %d, raw = %d\n", SOCK_DGRAM,
	       SOCK_STREAM, SOCK_RAW);
	printf("Protocols: tcp = %d, udp = %d\n\n", IPPROTO_TCP, IPPROTO_UDP);
	for (iter = addr; iter != NULL; iter = iter->ai_next) {
		printf("family = %d, socktype = %d, protocol = %d",
		       iter->ai_family, iter->ai_socktype, iter->ai_protocol);
		if (iter->ai_family == AF_INET) {
			char buf[128];
			struct sockaddr_in *tmp =
				(struct sockaddr_in *)iter->ai_addr;
			inet_ntop(AF_INET, &tmp->sin_addr, buf, sizeof(buf));
			printf(", ip = %s", buf);
		}
		printf("\n");
	}
	freeaddrinfo(addr);
	return 0;
}
