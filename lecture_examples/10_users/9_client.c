#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

int
main(int argc, const char **argv)
{
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == -1) {
		printf("error = %s\n", strerror(errno));
		return -1;
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	inet_aton("127.0.0.1", &addr.sin_addr);
	if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
		printf("connect error = %s\n", strerror(errno));
		close(sock);
		return -1;
	}
	int number;
	while (scanf("%d", &number) > 0) {
		if (write(sock, &number, sizeof(number)) == -1) {
			printf("error = %s\n", strerror(errno));
			continue;
		}
		printf("Sent %d\n", number);
		number = 0;
		int rc = read(sock, &number, sizeof(number));
		if (rc == 0) {
			printf("Closed connection\n");
			break;
		}
		if (rc == -1)
			printf("error = %s\n", strerror(errno));
		else
			printf("Received %d\n", number);
	}
	close(sock);
	return 0;
}
