#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>
#include <arpa/inet.h>

int
main(void)
{
	struct sockaddr_in in;
	in.sin_family = AF_INET;
	inet_aton("127.0.0.1", &in.sin_addr);
	in.sin_port = htons(3333);
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(s, (struct sockaddr *) &in, sizeof(in)) < 0)
		printf("connect error: %s\n", strerror(errno));
	else
		printf("connect success\n");
	close(s);
	return 0;
}
