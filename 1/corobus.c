#include "corobus.h"

#include "libcoro.h"
#include "rlist.h"

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
	memcpy(data, vector->data, sizeof(data[0]) * count);
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

struct wakeup_entry {
	struct rlist base;
	struct coro *coro;
};

struct wakeup_queue {
	struct rlist coros;
};

static void
wakeup_queue_suspend_this(struct wakeup_queue *queue)
{
	struct wakeup_entry entry;
	entry.coro = coro_this();
	rlist_add_tail_entry(&queue->coros, &entry, base);
	coro_suspend();
	rlist_del_entry(&entry, base);
}

static void
wakeup_queue_wakeup_first(struct wakeup_queue *queue)
{
	if (!rlist_empty(&queue->coros))
		coro_wakeup(rlist_first_entry(&queue->coros, struct wakeup_entry, base)->coro);
}

struct coro_bus_channel {
	size_t size_limit;
	struct wakeup_queue send_queue;
	struct wakeup_queue recv_queue;
	struct data_vector data;
};

struct coro_bus {
	struct coro_bus_channel **channels;
	int channel_count;
};

static enum coro_bus_error_code global_error = CORO_BUS_ERR_NONE;

enum coro_bus_error_code
coro_bus_errno(void)
{
	return global_error;
}

void
coro_bus_errno_set(enum coro_bus_error_code err)
{
	global_error = err;
}

struct coro_bus *
coro_bus_new(void)
{
	return calloc(1, sizeof(struct coro_bus));
}

void
coro_bus_delete(struct coro_bus *bus)
{
	for (int i = 0; i < bus->channel_count; ++i) {
		if (bus->channels[i] != NULL)
			coro_bus_channel_close(bus, i);
	}
	free(bus->channels);
	free(bus);
}

int
coro_bus_channel_open(struct coro_bus *bus, size_t size_limit)
{
	int idx = -1;
	for (int i = 0; i < bus->channel_count; ++i) {
		if (bus->channels[i] == NULL) {
			idx = i;
			break;
		}
	}
	if (idx < 0) {
		idx = bus->channel_count++;
		bus->channels = realloc(bus->channels, sizeof(bus->channels[0]) * bus->channel_count);
		memset(&bus->channels[idx], 0, sizeof(bus->channels[idx]));
	}
	struct coro_bus_channel *chan = calloc(1, sizeof(*chan));
	bus->channels[idx] = chan;
	rlist_create(&chan->send_queue.coros);
	rlist_create(&chan->recv_queue.coros);
	chan->size_limit = size_limit;
	return idx;
}

void
coro_bus_channel_close(struct coro_bus *bus, int channel)
{
	assert(channel >= 0 && channel < bus->channel_count);
	struct coro_bus_channel *chan = bus->channels[channel];
	assert(chan != NULL);

	struct wakeup_entry *entry;
	rlist_foreach_entry(entry, &chan->send_queue.coros, base)
		coro_wakeup(entry->coro);
	rlist_foreach_entry(entry, &chan->recv_queue.coros, base)
		coro_wakeup(entry->coro);

	free(chan->data.data);

	free(chan);
	bus->channels[channel] = NULL;
}

int
coro_bus_send(struct coro_bus *bus, int channel, unsigned data)
{
	while (coro_bus_try_send(bus, channel, data) != 0) {
		if (global_error != CORO_BUS_ERR_WOULD_BLOCK)
			return -1;
		struct coro_bus_channel *chan = bus->channels[channel];
		assert(chan->data.size >= chan->size_limit);
		wakeup_queue_suspend_this(&chan->send_queue);
	}
	struct coro_bus_channel *chan = bus->channels[channel];
	if (chan->data.size < chan->size_limit)
		wakeup_queue_wakeup_first(&chan->send_queue);
	return 0;
}

int
coro_bus_try_send(struct coro_bus *bus, int channel, unsigned data)
{
	if (channel < 0 || channel >= bus->channel_count) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
	struct coro_bus_channel *chan = bus->channels[channel];
	if (chan == NULL) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
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
	while (coro_bus_try_recv(bus, channel, data) != 0) {
		if (global_error != CORO_BUS_ERR_WOULD_BLOCK)
			return -1;
		struct coro_bus_channel *chan = bus->channels[channel];
		assert(chan->data.size == 0);
		wakeup_queue_suspend_this(&chan->recv_queue);
	}
	struct coro_bus_channel *chan = bus->channels[channel];
	if (chan->data.size > 0)
		wakeup_queue_wakeup_first(&chan->recv_queue);
	return 0;
}

int
coro_bus_try_recv(struct coro_bus *bus, int channel, unsigned *data)
{
	if (channel < 0 || channel >= bus->channel_count) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
	struct coro_bus_channel *chan = bus->channels[channel];
	if (chan == NULL) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
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
			if (chan->data.size == chan->size_limit) {
				wakeup_queue_suspend_this(&chan->send_queue);
				break;
			}
		}
	}
	return -1;
}

int
coro_bus_try_broadcast(struct coro_bus *bus, unsigned data)
{
	bool has_channels = false;
	for (int i = 0; i < bus->channel_count; ++i) {
		struct coro_bus_channel *chan = bus->channels[i];
		if (chan == NULL)
			continue;
		if (chan->data.size == chan->size_limit) {
			global_error = CORO_BUS_ERR_WOULD_BLOCK;
			return -1;
		}
		has_channels = true;
	}
	if (!has_channels) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
	for (int i = 0; i < bus->channel_count; ++i) {
		struct coro_bus_channel *chan = bus->channels[i];
		if (chan == NULL)
			continue;
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
	int rc;
	while ((rc = coro_bus_try_send_v(bus, channel, data, count)) < 0) {
		if (global_error != CORO_BUS_ERR_WOULD_BLOCK)
			return -1;
		struct coro_bus_channel *chan = bus->channels[channel];
		assert(chan->data.size >= chan->size_limit);
		wakeup_queue_suspend_this(&chan->send_queue);
	}
	struct coro_bus_channel *chan = bus->channels[channel];
	if (chan->data.size < chan->size_limit)
		wakeup_queue_wakeup_first(&chan->send_queue);
	return rc;
}

int
coro_bus_try_send_v(struct coro_bus *bus, int channel, const unsigned *data, unsigned count)
{
	if (channel < 0 || channel >= bus->channel_count) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
	struct coro_bus_channel *chan = bus->channels[channel];
	if (chan == NULL) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
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
	int rc;
	while ((rc = coro_bus_try_recv_v(bus, channel, data, capacity)) < 0) {
		if (global_error != CORO_BUS_ERR_WOULD_BLOCK)
			return -1;
		struct coro_bus_channel *chan = bus->channels[channel];
		assert(chan->data.size == 0);
		wakeup_queue_suspend_this(&chan->recv_queue);
	}
	struct coro_bus_channel *chan = bus->channels[channel];
	if (chan->data.size > 0)
		wakeup_queue_wakeup_first(&chan->recv_queue);
	return rc;
}

int
coro_bus_try_recv_v(struct coro_bus *bus, int channel, unsigned *data, unsigned capacity)
{
	if (channel < 0 || channel >= bus->channel_count) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
	struct coro_bus_channel *chan = bus->channels[channel];
	if (chan == NULL) {
		global_error = CORO_BUS_ERR_NO_CHANNEL;
		return -1;
	}
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
