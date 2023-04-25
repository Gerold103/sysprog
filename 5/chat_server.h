#pragma once

#include <stdint.h>

struct chat_server;

/**
 * Create a new chat server. No bind, no listen, just allocate and
 * initialize it.
 */
struct chat_server *
chat_server_new(void);

/** Free all server's resources. */
void
chat_server_delete(struct chat_server *server);

/**
 * Try to listen for new clients on the given port.
 *
 * @param server Chat server.
 * @param port Port to listen on.
 *
 * @retval 0 Success.
 * @retval !=0 Error code.
 *     - CHAT_ERR_PORT_BUSY - the port is already busy.
 *     - CHAT_ERR_ALREADY_STARTED - the server is already listening.
 *     - CHAT_ERR_SYS - a system error, check errno.
 */
int
chat_server_listen(struct chat_server *server, uint16_t port);

/**
 * Pop a next pending chat message. The returned message has to be
 * freed using chat_message_delete().
 *
 * @param server Chat server.
 *
 * @retval not-NULL A message.
 * @retval NULL No more messages yet.
 */
struct chat_message *
chat_server_pop_next(struct chat_server *server);

/**
 * Wait for any update on any of the sockets for the given timeout
 * and do this update.
 *
 * @param server Chat server.
 * @param timeout Timeout in seconds to wait for.
 *
 * @retval 0 Success.
 * @retval !=0 Error code.
 *     - CHAT_ERR_TIMEOUT - no updates, timed out.
 *     - CHAT_ERR_NOT_STARTED - the server is not listening yet.
 *     - CHAT_ERR_SYS - a system error, check errno.
 */
int
chat_server_update(struct chat_server *server, double timeout);

/**
 * Get server's descriptor suitable for event loops like poll/epoll/kqueue. This
 * is useful when want to embed the server into some external event loop. For
 * example, to join it with reading stdin.
 *
 * @retval >=0 A valid descriptor.
 * @retval -1 No descriptor.
 */
int
chat_server_get_descriptor(const struct chat_server *server);

/**
 * Get the server's own socket descriptor. Not a multiplexing descriptor like
 * an epoll or kqueue one, but the original listening descriptor.
 *
 * @retval >=0 A valid descriptor.
 * @retval -1 No descriptor.
 */
int
chat_server_get_socket(const struct chat_server *server);

/**
 * Get a mask of chat_event values wanted by the server. Needed together with
 * server's descriptor for any waiting in poll/epoll/kqueue.
 *
 * @retval !=0 Event mask to wait for.
 * @retval 0 No events.
 */
int
chat_server_get_events(const struct chat_server *server);

/**
 * Feed a message to the server to broadcast to all clients.
 *
 * @param server Chat server.
 * @param msg Message.
 * @param msg_size Size of the message.
 *
 * @retval 0 Success.
 * @retval !=0 Error code.
 *     - CHAT_ERR_NOT_IMPLEMENTED - not implemented.
 *     - CHAT_ERR_NOT_STARTED - the server is not listening yet.
 */
int
chat_server_feed(struct chat_server *server, const char *msg,
		 uint32_t msg_size);
