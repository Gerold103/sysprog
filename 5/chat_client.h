#pragma once

#include <stdint.h>

struct chat_client;

/**
 * Create a new chat client. No bind, no listen, just allocate and
 * initialize it.
 */
struct chat_client *
chat_client_new(const char *name);

/** Free all client's resources. */
void
chat_client_delete(struct chat_client *client);

/**
 * Try to connect to the given address.
 *
 * @param client Chat client.
 * @param addr Address to connect to, like 'localhost:1234',
 *     '127.0.0.1:3313', '192.168.0.1:4567', etc.
 *
 * @retval 0 Success.
 * @retval !=0 Error code.
 *     - CHAT_ERR_ALREADY_STARTED - the client is already connected.
 *     - CHAT_ERR_NO_ADDR - the addr couldn't be resolved to any IP.
 *     - CHAT_ERR_SYS - a system error, check errno.
 */
int
chat_client_connect(struct chat_client *client, const char *addr);

/**
 * Pop a next pending chat message. The returned message has to be
 * freed using chat_message_delete().
 *
 * @param client Chat client.
 *
 * @retval not-NULL A message.
 * @retval NULL No more messages yet.
 */
struct chat_message *
chat_client_pop_next(struct chat_client *client);

/**
 * Wait for any update for the given timeout and do this update.
 *
 * @param client Chat client.
 * @param timeout Timeout in seconds to wait for.
 *
 * @retval 0 Success.
 * @retval !=0 Error code.
 *     - CHAT_ERR_TIMEOUT - no updates, timed out.
 *     - CHAT_ERR_NOT_STARTED - the client is not connected yet.
 *     - CHAT_ERR_SYS - a system error, check errno.
 */
int
chat_client_update(struct chat_client *client, double timeout);

/**
 * Get client's descriptor suitable for event loops like poll/epoll/kqueue. This
 * is useful when want to embed the client into some external event loop. For
 * example, to join it with reading stdin.
 *
 * @retval >=0 A valid descriptor.
 * @retval -1 No descriptor.
 */
int
chat_client_get_descriptor(const struct chat_client *client);

/**
 * Get a mask of chat_event values wanted by the client. Needed together with
 * client's descriptor for any waiting in poll/epoll/queue.
 *
 * @retval !=0 Event mask to wait for.
 * @retval 0 No events.
 */
int
chat_client_get_events(const struct chat_client *client);

/**
 * Feed a message to the client.
 *
 * @param client Chat client.
 * @param msg Message.
 * @param msg_size Size of the message.
 *
 * @retval 0 Success.
 * @retval !=0 Error code.
 *     - CHAT_ERR_NOT_STARTED - the client is not connected yet.
 */
int
chat_client_feed(struct chat_client *client, const char *msg,
		 uint32_t msg_size);
