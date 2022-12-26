#include "chat.h"
#include "chat_client.h"

#include <stdlib.h>
#include <unistd.h>

struct chat_client {
	/** Socket connected to the server. */
	int socket;
	/** Array of received messages. */
	/* ... */
	/** Output buffer. */
	/* ... */
	/* PUT HERE OTHER MEMBERS */
};

struct chat_client *
chat_client_new(const char *name)
{
	/* Ignore 'name' param if don't want to support it for +5 points. */
	(void)name;

	struct chat_client *client = calloc(1, sizeof(*client));
	client->socket = -1;

	/* IMPLEMENT THIS FUNCTION */

	return client;
}

void
chat_client_delete(struct chat_client *client)
{
	if (client->socket >= 0)
		close(client->socket);

	/* IMPLEMENT THIS FUNCTION */

	free(client);
}

int
chat_client_connect(struct chat_client *client, const char *addr)
{
	/*
	 * 1) Use getaddrinfo() to resolve addr to struct sockaddr_in.
	 * 2) Create a client socket (function socket()).
	 * 3) Connect it by the found address (function connect()).
	 */
	/* IMPLEMENT THIS FUNCTION */
	(void)client;
	(void)addr;

	return CHAT_ERR_NOT_IMPLEMENTED;
}

struct chat_message *
chat_client_pop_next(struct chat_client *client)
{
	/* IMPLEMENT THIS FUNCTION */
	(void)client;
	return NULL;
}

int
chat_client_update(struct chat_client *client, double timeout)
{
	/*
	 * The easiest way to wait for updates on a single socket with a timeout
	 * is to use poll(). Epoll is good for many sockets, poll is good for a
	 * few.
	 *
	 * You create one struct pollfd, fill it, call poll() on it, handle the
	 * events (do read/write).
	 */
	(void)client;
	(void)timeout;
	return CHAT_ERR_NOT_IMPLEMENTED;
}

int
chat_client_get_descriptor(const struct chat_client *client)
{
	return client->socket;
}

int
chat_client_get_events(const struct chat_client *client)
{
	/*
	 * IMPLEMENT THIS FUNCTION - add OUTPUT event if has non-empty output
	 * buffer.
	 */
	(void)client;
	return CHAT_EVENT_INPUT;
}

int
chat_client_feed(struct chat_client *client, const char *msg, uint32_t msg_size)
{
	/* IMPLEMENT THIS FUNCTION */
	(void)client;
	(void)msg;
	(void)msg_size;
	return CHAT_ERR_NOT_IMPLEMENTED;
}
