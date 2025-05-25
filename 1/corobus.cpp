#include "corobus.h"

#include "libcoro.h"
#include "rlist.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// One coroutine waiting to be woken up in a list of other suspended coros.
struct wakeup_entry {
	rlist base;
	coro *coro;
};

// A queue of suspended coros waiting to be woken up.
class wakeup_queue {
public:
	wakeup_queue()
	{
		rlist_create(&m_coros);
	}

	~wakeup_queue()
	{
		wakeup_entry *entry;
		while (!rlist_empty(&m_coros)) {
			entry = rlist_shift_entry(&m_coros, wakeup_entry, base);
			coro_wakeup(entry->coro);
		}
	}

	// Suspend the current coroutine until it is woken up.
	void
	suspend_this()
	{
		wakeup_entry entry;
		entry.coro = coro_this();
		rlist_add_tail_entry(&m_coros, &entry, base);
		coro_suspend();
		rlist_del_entry(&entry, base);
	}

	// Wakeup the first coroutine in the queue.
	void
	wakeup_first()
	{
		if (rlist_empty(&m_coros))
			return;
		wakeup_entry *entry = rlist_first_entry(&m_coros, wakeup_entry, base);
		coro_wakeup(entry->coro);
	}

private:
	rlist m_coros;
};

struct coro_bus_channel {
	coro_bus_channel(size_t size_limit)
		: m_size_limit(size_limit)
	{
	}

	// Channel max capacity.
	const size_t m_size_limit;
	// Coroutines waiting until the channel is not full.
	wakeup_queue m_send_queue;
	// Coroutines waiting until the channel is not empty.
	wakeup_queue m_recv_queue;
	// Message queue.
	std::vector<unsigned> m_data;
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


coro_bus::coro_bus()
{
	// IMPLEMENT THIS FUNCTION
}

coro_bus::~coro_bus()
{
	// IMPLEMENT THIS FUNCTION
}

int
coro_bus::open_channel(size_t size_limit)
{
	// IMPLEMENT THIS FUNCTION
	(void)size_limit;
	coro_bus_errno_set(CORO_BUS_ERR_NOT_IMPLEMENTED);

	// One of the tests will force you to reuse the channel descriptors. It means,
	// that if your maximal channel descriptor is N, and you have any free descriptor
	// in the range 0-N, then you should open the new channel on that old descriptor.
	//
	// A more precise instruction - check if any of the m_channels[i]
	// with i = 0 -> m_channels.size() is free (== nullptr). If yes - reuse the slot.
	// Don't grow the m_channels array, when have space in it.
	return -1;
}

void
coro_bus::close_channel(int channel)
{
	// IMPLEMENT THIS FUNCTION
	(void)channel;
}

int
coro_bus::send(int channel, unsigned data)
{
	// IMPLEMENT THIS FUNCTION
	(void)channel;
	(void)data;

	// Try sending in a loop, until success. If error, then check which one is that.
	// If 'wouldblock', then suspend this coroutine and try again when woken up.
	//
	// If see the channel has space, then wakeup the first coro in the send-queue.
	// That is needed so when there is enough space for many messages, and many
	// coroutines are waiting, they would then wake each other up one by one as long
	// as there is still space.
	coro_bus_errno_set(CORO_BUS_ERR_NOT_IMPLEMENTED);
	return -1;
}

int
coro_bus::try_send(int channel, unsigned data)
{
	// IMPLEMENT THIS FUNCTION
	(void)channel;
	(void)data;

	// Append data if has space. Otherwise 'wouldblock' error. Wakeup the first coro
	// in the recv-queue! To let it know there is data.
	coro_bus_errno_set(CORO_BUS_ERR_NOT_IMPLEMENTED);
	return -1;
}

int
coro_bus::recv(int channel, unsigned *data)
{
	// IMPLEMENT THIS FUNCTION
	(void)channel;
	(void)data;
	coro_bus_errno_set(CORO_BUS_ERR_NOT_IMPLEMENTED);
	return -1;
}

int
coro_bus::try_recv(int channel, unsigned *data)
{
	// IMPLEMENT THIS FUNCTION
	(void)channel;
	(void)data;
	coro_bus_errno_set(CORO_BUS_ERR_NOT_IMPLEMENTED);
	return -1;
}


#if NEED_BROADCAST

int
coro_bus::broadcast(unsigned data)
{
	// IMPLEMENT THIS FUNCTION
	(void)data;
	coro_bus_errno_set(CORO_BUS_ERR_NOT_IMPLEMENTED);
	return -1;
}

int
coro_bus::try_broadcast(unsigned data)
{
	// IMPLEMENT THIS FUNCTION
	(void)data;
	coro_bus_errno_set(CORO_BUS_ERR_NOT_IMPLEMENTED);
	return -1;
}

#endif

#if NEED_BATCH

int
coro_bus::send_v(int channel, const unsigned *data, unsigned count)
{
	// IMPLEMENT THIS FUNCTION
	(void)channel;
	(void)data;
	(void)count;
	coro_bus_errno_set(CORO_BUS_ERR_NOT_IMPLEMENTED);
	return -1;
}

int
coro_bus::try_send_v(int channel, const unsigned *data, unsigned count)
{
	// IMPLEMENT THIS FUNCTION
	(void)channel;
	(void)data;
	(void)count;
	coro_bus_errno_set(CORO_BUS_ERR_NOT_IMPLEMENTED);
	return -1;
}

int
coro_bus::recv_v(int channel, unsigned *data, unsigned capacity)
{
	// IMPLEMENT THIS FUNCTION
	(void)channel;
	(void)data;
	(void)capacity;
	coro_bus_errno_set(CORO_BUS_ERR_NOT_IMPLEMENTED);
	return -1;
}

int
coro_bus::try_recv_v(int channel, unsigned *data, unsigned capacity)
{
	// IMPLEMENT THIS FUNCTION
	(void)channel;
	(void)data;
	(void)capacity;
	coro_bus_errno_set(CORO_BUS_ERR_NOT_IMPLEMENTED);
	return -1;
}

#endif
