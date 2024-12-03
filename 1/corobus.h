#pragma once

/**
 * Here you should specify which bonuses do you want via the macros. It is important to
 * define these macros here, in the header, because it is used by tests.
 */
#define NEED_BROADCAST 0
#define NEED_BATCH 0

enum coro_bus_error_code {
	CORO_BUS_ERR_NONE = 0,
	CORO_BUS_ERR_NO_CHANNEL,
	CORO_BUS_ERR_WOULD_BLOCK,
};

/**
 * Get the latest error happened in coro_bus.
 */
enum coro_bus_error_code
coro_bus_errno(void);

/**
 * Create a new messaging bus with no channels in it.
 */
coro_bus *
coro_bus_new(void);

/**
 * Create a channel inside the bus.
 * @param bus The bus to create the channel in.
 * @param size_limit Maximum messages the channel can hold in
 *     memory until they are received.
 *
 * @retval >0 Descriptor of the channel. It must be passed to the
 *     send/recv functions.
 */
int
coro_bus_channel_open(coro_bus *bus, size_t size_limit);

/**
 * Destroy the channel identified by the given channel descriptor.
 * All pending messages of the channel are deleted and lost.
 * @param bus Bus to destroy the channel in.
 * @param channel Descriptor of the channel to destroy.
 */
void
coro_bus_channel_destroy(coro_bus *bus, int channel);

/**
 * Send the given message to the specified channel. If the channel
 * is full, the function should suspend the current coroutine and
 * retry until success or until the channel is gone.
 * @param bus Bus where the channel is located.
 * @param channel Descriptor of the channel to send data to.
 * @param data Data to send.
 *
 * @retval 0 Success.
 * @retval -1 Error. Check coro_bus_errno() for reason.
 *     - CORO_BUS_ERR_NO_CHANNEL - the channel doesn't exist.
 */
int
coro_bus_send(coro_bus *bus, int channel, unsigned data);

/**
 * Same as coro_bus_send(), but if the channel is full, the
 * function immediately returns. It never suspends the current
 * coroutine.
 * @param bus Bus where the channel is located.
 * @param channel Descriptor of the channel to send data to.
 * @param data Data to send.
 *
 * @retval 0 Success.
 * @retval -1 Error. Check coro_bus_errno() for reason.
 *     - CORO_BUS_ERR_NO_CHANNEL - the channel doesn't exist.
 *     - CORO_BUS_ERR_WOULD_BLOCK - the channel is full.
 */
int
coro_bus_try_send(coro_bus *bus, int channel, unsigned data);

/**
 * Recv a message from the specified channel. If the channel is
 * empty, the function should suspend the current coroutine and
 * retry until success or until the channel is gone.
 * @param bus Bus where the channel is located.
 * @param channel Descriptor of the channel to send data to.
 * @param data Output parameter to save the data to.
 *
 * @retval 0 Success. Data output is filled with the received
 *     message.
 * @retval -1 Error. Check coro_bus_errno() for reason.
 *     - CORO_BUS_ERR_NO_CHANNEL - the channel doesn't exist.
 */
int
coro_bus_recv(coro_bus *bus, int channel, unsigned *data);

/**
 * Same as coro_bus_recv(), but if the channel is empty, the
 * function immediately returns. It never suspends the current
 * coroutine.
 * @param bus Bus where the channel is located.
 * @param channel Descriptor of the channel to send data to.
 * @param data Output parameter to save the data to.
 *
 * @retval 0 Success. Data output is filled with the received
 *     message.
 * @retval -1 Error. Check coro_bus_errno() for reason.
 *     - CORO_BUS_ERR_NO_CHANNEL - the channel doesn't exist.
 *     - CORO_BUS_ERR_WOULD_BLOCK - the channel is empty.
 */
int
coro_bus_try_recv(coro_bus *bus, int channel, unsigned *data);


#if NEED_BROADCAST

int
coro_bus_broadcast(coro_bus *bus, unsigned data);

int
coro_bus_try_broadcast(coro_bus *bus, unsigned data);

#endif

#if NEED_BATCH

int
coro_bus_try_send_v(coro_bus *bus, int channel, const unsigned *data, unsigned count);

int
coro_bus_send_v(coro_bus *bus, int channel, const unsigned *data, unsigned count);

int
coro_bus_try_recv_v(coro_bus *bus, int channel, unsigned *data, unsigned capacity);

int
coro_bus_recv_v(coro_bus *bus, int channel, unsigned *data, unsigned capacity);

#endif
