#include "corobus.h"

#include "libcoro.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct data_vector {
	unsigned *data;
	size_t size;
	size_t capacity;
};

static void
data_vector_append_many(struct data_vector *vector, const unsigned *data, size_t count)
{
	if (vector->size + count > vector->capacity) {
		if (vector->capacity == 0)
			vector->capacity = 4;
		else
			vector->capacity *= 2;
		if (vector->capacity < vector->size + count)
			vector->capacity = vector->size + count;
		vector->data = realloc(vector->data, sizeof(vector->data[0]) * vector->capacity);
	}
	memcpy(&vector->data[vector->size], data, sizeof(data[0]) * count);
	vector->size += count;
}

static void
data_vector_append(struct data_vector *vector, unsigned data)
{
	data_vector_append_many(vector, &data, 1);
}

static void
data_vector_pop_first_many(struct data_vector *vector, unsigned *data, size_t count)
{
	assert(count <= vector->size);
	memcpy(vector->data, data, sizeof(data[0]) * count);
	vector->size -= count;
	memmove(vector->data, &vector->data[count], vector->size * sizeof(vector->data[0]));
}

static unsigned
data_vector_pop_first(struct data_vector *vector)
{
	unsigned data = 0;
	data_vector_pop_first_many(vector, &data, 1);
	return data;
}

struct wakeup_queue {
	struct coro **coros;
	size_t size;
	size_t capacity;
};

static void
wakeup_queue_suspend_this(struct wakeup_queue *queue)
{
	if (queue->size == queue->capacity) {
		if (queue->capacity == 0)
			queue->capacity = 4;
		else
			queue->capacity *= 2;
		queue->coros = realloc(queue->coros, sizeof(queue->coros[0]) * queue->capacity);
	}
	queue->coros[queue->size++] = coro_this();
}

static void
wakeup_queue_wakeup_first(struct wakeup_queue *queue)
{
	if (queue->size == 0)
		return;
	coro_wakeup(queue->coros[0]);
	--queue->size;
	memmove(queue->coros, &queue->coros[1], queue->size * sizeof(queue->coros[0]));
}

struct coro_bus_channel {
	bool is_used;
	size_t size_limit;
	struct wakeup_queue send_queue;
	struct wakeup_queue recv_queue;
	struct data_vector data;
};

struct coro_bus {
	struct coro_bus_channel *channels;
	int channel_count;
};

static enum coro_bus_error_code global_error = CORO_BUS_ERR_NONE;

enum coro_bus_error_code
coro_bus_errno(void)
{
	return global_error;
}

struct coro_bus *
coro_bus_new(void)
{
	return calloc(1, sizeof(struct coro_bus));
}

void
coro_bus_delete(struct coro_bus *bus)
{
	for (int i = 0; i < bus->channel_count; ++i)
		coro_bus_channel_destroy(bus, i + 1);
	free(bus->channels);
	free(bus);
}

int
coro_bus_channel_open(struct coro_bus *bus, size_t size_limit)
{
	int idx = -1;
	for (int i = 0; i < bus->channel_count; ++i) {
		if (!bus->channels[i].is_used) {
			idx = i;
			break;
		}
	}
	if (idx < 0) {
		idx = bus->channel_count++;
		bus->channels = realloc(bus->channels, sizeof(bus->channels[0]) * bus->channel_count);
		memset(&bus->channels[idx], 0, sizeof(bus->channels[idx]));
	}
	struct coro_bus_channel *chan = &bus->channels[idx];
	chan->is_used = true;
	chan->size_limit = size_limit;
	return idx + 1;
}

void
coro_bus_channel_destroy(struct coro_bus *bus, int channel)
{
	assert(channel > 0 && channel <= channel_count);
	int idx = channel - 1;
	struct coro_bus_channel *chan = &bus->channels[idx];
	assert(chan->is_used);
	chan->is_used = false;

	// TODO: check everywhere that the channel is in use.

	for (size_t i = 0; i < chan->send_queue.size; ++i)
		coro_wakeup(chan->send_queue.coros[i]);
	free(chan->send_queue.coros);

	for (size_t i = 0; i < chan->recv_queue.size; ++i)
		coro_wakeup(chan->recv_queue.coros[i]);
	free(chan->recv_queue.coros);

	free(chan->data.data);

	free(chan);
	bus->channels[idx] = NULL;
}

int
coro_bus_send(struct coro_bus *bus, int channel, unsigned data)
{
	int idx = channel - 1;
	while (coro_bus_try_send(bus, channel, data) != 0) {
		if (global_error != CORO_BUS_ERR_WOULD_BLOCK)
			return -1;
		struct coro_bus_channel *chan = bus->channels[idx];
		assert(chan->data.size >= chan->size_limit);
		wakeup_queue_suspend_this(&chan->send_queue);
	}
	struct coro_bus_channel *chan = bus->channels[idx];
	if (chan->data.size < chan->size_limit)
		wakeup_queue_wakeup_first(&chan->send_queue);
	return 0;
}

int
coro_bus_try_send(struct coro_bus *bus, int channel, unsigned data)
{
	int idx = channel - 1;
	if (idx < 0 || idx >= channel_count) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
	struct coro_bus_channel *chan = bus->channels[idx];
	if (chan->data.size >= chan->size_limit) {
		global_error = CORO_BUS_ERR_WOULD_BLOCK;
		return -1;
	}
	data_vector_append(&chan->data, data);
	wakeup_queue_wakeup_first(&chan->recv_queue);
	return 0;
}

int
coro_bus_recv(struct coro_bus *bus, int channel, unsigned *data)
{
	int idx = channel - 1;
	while (coro_bus_try_recv(bus, channel, data) != 0) {
		if (global_error != CORO_BUS_ERR_WOULD_BLOCK)
			return -1;
		struct coro_bus_channel *chan = bus->channels[idx];
		assert(chan->data.size == 0);
		wakeup_queue_suspend_this(&chan->recv_queue);
	}
	struct coro_bus_channel *chan = bus->channels[idx];
	if (chan->data.size > 0)
		wakeup_queue_wakeup_first(&chan->recv_queue);
	return 0;
}

int
coro_bus_try_recv(struct coro_bus *bus, int channel, unsigned *data)
{
	int idx = channel - 1;
	if (idx < 0 || idx >= channel_count) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
	struct coro_bus_channel *chan = bus->channels[idx];
	if (chan->data.size == 0) {
		global_error = CORO_BUS_ERR_WOULD_BLOCK;
		return -1;
	}
	*data = data_vector_pop_first(&chan->data);
	wakeup_queue_wakeup_first(&chan->send_queue);
	return 0;
}


#if NEED_BROADCAST

int
coro_bus_broadcast(struct coro_bus *bus, unsigned data)
{
	while (coro_bus_try_broadcast(bus, data) != 0) {
		if (global_error != CORO_BUS_ERR_WOULD_BLOCK)
			return -1;
		for (int i = 0; i < bus->channel_count; ++i) {
			struct coro_bus_channel *chan = bus->channels[i];
			if (chan->size == chan->size_limit) {
				wakeup_queue_suspend_this(&chan->send_queue);
				break;
			}
		}
	}
}

int
coro_bus_try_broadcast(struct coro_bus *bus, unsigned data)
{
	if (bus->channel_count == 0) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
	for (int i = 0; i < bus->channel_count; ++i) {
		struct coro_bus_channel *chan = bus->channels[i];
		if (chan->size == chan->size_limit) {
			global_error = CORO_BUS_ERR_WOULD_BLOCK;
			return -1;
		}
	}
	for (int i = 0; i < bus->channel_count; ++i) {
		struct coro_bus_channel *chan = bus->channels[i];
		data_vector_append(&chan->data, data);
		wakeup_queue_wakeup_first(&chan->recv_queue);
	}
	return 0;
}

#endif

#if NEED_BATCH

int
coro_bus_send_v(struct coro_bus *bus, int channel, const unsigned *data, unsigned count)
{
	int idx = channel - 1;
	int rc;
	while ((rc = coro_bus_try_send_v(bus, channel, data, count)) < 0) {
		if (global_error != CORO_BUS_ERR_WOULD_BLOCK)
			return -1;
		struct coro_bus_channel *chan = bus->channels[idx];
		assert(chan->data.size >= chan->size_limit);
		wakeup_queue_suspend_this(&chan->send_queue);
	}
	struct coro_bus_channel *chan = bus->channels[idx];
	if (chan->data.size < chan->size_limit)
		wakeup_queue_wakeup_first(&chan->send_queue);
	return rc;
}

int
coro_bus_try_send_v(struct coro_bus *bus, int channel, const unsigned *data, unsigned count)
{
	int idx = channel - 1;
	if (idx < 0 || idx >= channel_count) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
	struct coro_bus_channel *chan = bus->channels[idx];
	if (chan->data.size >= chan->size_limit) {
		global_error = CORO_BUS_ERR_WOULD_BLOCK;
		return -1;
	}
	int to_move;
	if (chan->data.size + count <= chan->size_limit)
		to_move = count;
	else
		to_move = chan->size_limit - chan->data.size;
	data_vector_append_many(&chan->data, data, to_move);
	wakeup_queue_wakeup_first(&chan->recv_queue);
	return to_move;
}

int
coro_bus_recv_v(struct coro_bus *bus, int channel, unsigned *data, unsigned capacity)
{
	int idx = channel - 1;
	int rc;
	while ((rc = coro_bus_try_recv_v(bus, channel, data, capacity)) < 0) {
		if (global_error != CORO_BUS_ERR_WOULD_BLOCK)
			return -1;
		struct coro_bus_channel *chan = bus->channels[idx];
		assert(chan->data.size == 0);
		wakeup_queue_suspend_this(&chan->recv_queue);
	}
	struct coro_bus_channel *chan = bus->channels[idx];
	if (chan->data.size > 0)
		wakeup_queue_wakeup_first(&chan->recv_queue);
	return rc;
}

int
coro_bus_try_recv_v(struct coro_bus *bus, int channel, unsigned *data, unsigned capacity)
{
	int idx = channel - 1;
	if (idx < 0 || idx >= channel_count) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
	struct coro_bus_channel *chan = bus->channels[idx];
	if (chan->data.size == 0) {
		global_error = CORO_BUS_ERR_WOULD_BLOCK;
		return -1;
	}
	int to_move;
	if (chan->data.size >= capacity)
		to_move = capacity;
	else
		to_move = chan->data.size;
	data_vector_pop_first_many(&chan->data, data, to_move);
	wakeup_queue_wakeup_first(&chan->send_queue);
	return to_move;
}

#endif
