#include "chat.h"
#include "chat_server.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static int
port_from_str(const char *str, uint16_t *port)
{
	errno = 0;
	char *end = NULL;
	long res = strtol(str, &end, 10);
	if (res == 0 && errno != 0)
		return -1;
	if (*end != 0)
		return -1;
	if (res > UINT16_MAX || res < 0)
		return -1;
	*port = (uint16_t)res;
	return 0;
}

int
main(int argc, char **argv)
{
	if (argc < 2) {
		printf("Expected a port to listen on\n");
		return -1;
	}
	uint16_t port = 0;
	int rc = port_from_str(argv[1], &port);
	if (rc != 0) {
		printf("Invalid port\n");
		return -1;
	}
	struct chat_server *serv = chat_server_new();
	rc = chat_server_listen(serv, port);
	if (rc != 0) {
		printf("Couldn't listen: %d\n", rc);
		chat_server_delete(serv);
		return -1;
	}
#if NEED_SERVER_FEED
	/*
	 * If want +5 points, then do similarly to the client_exe - create 2
	 * pollfd objects, wait on STDIN_FILENO and on
	 * chat_server_get_descriptor(), process events, etc.
	 */
#else
	/*
	 * The basic implementation without server messages. Just serving
	 * clients.
	 */
	while (true) {
		int rc = chat_server_update(serv, -1);
		if (rc != 0) {
			printf("Update error: %d\n", rc);
			break;
		}
		/* Flush all the pending messages to the standard output. */
		struct chat_message *msg;
		while ((msg = chat_server_pop_next(serv)) != NULL) {
#if NEED_AUTHOR
			printf("%s: %s\n", msg->author, msg->data);
#else
			printf("%s\n", msg->data);
#endif
			chat_message_delete(msg);
		}
	}
#endif
	chat_server_delete(serv);
	return 0;
}
