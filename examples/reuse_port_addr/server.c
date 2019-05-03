#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>
#include <arpa/inet.h>
#include <stdbool.h>

int
create_server(bool reuse_addr, bool reuse_port)
{
	struct sockaddr_in in;
	in.sin_family = AF_INET;
	inet_aton("127.0.0.1", &in.sin_addr);
	in.sin_port = htons(3333);
	int s = socket(AF_INET, SOCK_STREAM, 0);
	assert(s >= 0);
	int rc, value = 1;
	if (reuse_addr) {
		printf("reuse addr is set\n");
		rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &value,
				sizeof(int));
		assert(rc == 0);
	}
	if (reuse_port) {
		printf("reuse port is set\n");
		rc = setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &value,
				sizeof(int));
		assert(rc == 0);
	}
	if (bind(s, (struct sockaddr *) &in, sizeof(in)) != 0) {
		printf("bind error: %s\n", strerror(errno));
		close(s);
		return -1;
	}
	printf("bind success\n");
	if (listen(s, 128) < 0) {
		printf("listen error: %s\n", strerror(errno));
		close(s);
		return -1;
	}
	printf("listen success\n");
	return s;
}

int
main(int argc, char **argv)
{
	bool reuse_addr = false, reuse_port = false;
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "reuse_addr") == 0)
			reuse_addr = true;
		else if (strcmp(argv[i], "reuse_port") == 0)
			reuse_port = true;
	}
	int s = create_server(reuse_addr, reuse_port);
	if (s < 0)
		return -1;
	int c = accept(s, NULL, NULL);
	if (c < 0) {
		printf("accept error: %s\n", strerror(errno));
	} else {
		printf("accept success\n");
		close(c);
	}
	close(s);
	return 0;
}
