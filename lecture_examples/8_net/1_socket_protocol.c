#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

void
try_protocol(int type, int protocol, const char *protocol_name)
{
	int sock = socket(AF_INET, type, protocol);
	if (sock == -1) {
		printf("%s: error = %s\n", protocol_name, strerror(errno));
	} else {
		printf("%s: success\n", protocol_name);
		close(sock);
	}
}

void
try_type(int type, const char *type_name)
{
	printf("\nTry %s type\n", type_name);
	try_protocol(type, IPPROTO_TCP, "TCP");
	try_protocol(type, IPPROTO_IP, "IP");
	try_protocol(type, IPPROTO_SCTP, "SCTP");
	try_protocol(type, IPPROTO_RAW, "RAW");
	try_protocol(type, IPPROTO_UDP, "UDP");
}

int
main()
{
	try_type(SOCK_DGRAM, "DGRAM");
	try_type(SOCK_RAW, "RAW");
	try_type(SOCK_STREAM, "STREAM");
	return 0;
}
