#include "iocoro.h"

#include <cassert>
#include <cstring>
#include <sys/epoll.h>
#include <sys/eventfd.h>

static constexpr int theEpollBatchSize = 128;

std::atomic_int IOCoroutinePromise::theCount{0};
std::atomic_int IOTask::theCount{0};

//////////////////////////////////////////////////////////////////////////////////////////

AsyncOperation::AsyncOperation(IOTask *sub)
	: myTask(sub)
{
}

bool
AsyncOperation::await_suspend(
	std::coroutine_handle<> coro)
{
	assert(myTask->myAsyncOp == nullptr);
	myCoro = coro;
	myTask->myAsyncOp = this;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

AsyncRecv::AsyncRecv(
	IOTask *sub,
	void *data,
	size_t size)
	: AsyncOperation(sub)
	, myData(data)
	, mySize(size)
	, myRes(-1)
{
	execute();
}

void
AsyncRecv::execute()
{
	if ((myTask->myEventsReady & IO_EVENT_READ) == 0)
		return;
	myRes = recv(myTask->myFd, myData, mySize, 0);
	if (myRes >= 0)
		return;
	assert(errno == EWOULDBLOCK);
	// The event is consumed, no more data to read. Wait for a new event.
	myTask->myEventsReady &= ~IO_EVENT_READ;
}

bool
AsyncRecv::onIOEvent()
{
	if ((myTask->myEventsReady & IO_EVENT_READ) == 0)
	{
		if (myTask->myState == IO_TASK_STATE_DELETING)
		{
			// Cancellation.
			myRes = -1;
			myCoro.resume();
			return true;
		}
		return false;
	}
	execute();
	// Could be a spurious wakeup.
	if (myRes < 0)
		return false;
	myCoro.resume();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

AsyncSend::AsyncSend(
	IOTask *sub,
	const void *data,
	size_t size)
	: AsyncOperation(sub)
	, myData(data)
	, mySize(size)
	, myRes(-1)
{
	execute();
}

void
AsyncSend::execute()
{
	if ((myTask->myEventsReady & IO_EVENT_WRITE) == 0)
		return;
	myRes = send(myTask->myFd, myData, mySize, 0);
	if (myRes >= 0)
		return;
	assert(errno == EWOULDBLOCK);
	// Can't write anymore. Need to wait for a new write-event.
	myTask->myEventsReady &= ~IO_EVENT_WRITE;
}

bool
AsyncSend::onIOEvent()
{
	if ((myTask->myEventsReady & IO_EVENT_WRITE) == 0)
	{
		if (myTask->myState == IO_TASK_STATE_DELETING)
		{
			// Cancellation.
			myRes = -1;
			myCoro.resume();
			return true;
		}
		return false;
	}
	execute();
	// Could be a spurious wakeup.
	if (myRes < 0)
		return false;
	myCoro.resume();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

AsyncAccept::AsyncAccept(
	IOTask *sub,
	sockaddr *addr,
	socklen_t *size)
	: AsyncOperation(sub)
	, myAddr(addr)
	, mySize(size)
	, myRes(-1)
{
	execute();
}

void
AsyncAccept::execute()
{
	if ((myTask->myEventsReady & IO_EVENT_READ) == 0)
		return;
	myRes = accept(myTask->myFd, myAddr, mySize);
	if (myRes >= 0)
		return;
	assert(errno == EWOULDBLOCK);
	// Can't accept anymore. Need to wait for a new read-event.
	myTask->myEventsReady &= ~IO_EVENT_READ;
}

bool
AsyncAccept::onIOEvent()
{
	if ((myTask->myEventsReady & IO_EVENT_READ) == 0)
	{
		if (myTask->myState == IO_TASK_STATE_DELETING)
		{
			// Cancellation.
			myRes = -1;
			myCoro.resume();
			return true;
		}
		return false;
	}
	execute();
	// Could be a spurious wakeup.
	if (myRes < 0)
		return false;
	myCoro.resume();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

AsyncConnect::AsyncConnect(
	IOTask *sub,
	const sockaddr *addr,
	socklen_t size)
	: AsyncOperation(sub)
	, myIsDone(false)
	, myRes(-1)
{
	int rc = connect(myTask->myFd, addr, size);
	if (rc == 0)
	{
		// Event async connect might succeed instantly.
		myIsDone = true;
		myRes = 0;
		return;
	}
	// Connect is started. When the socket gets writable, it means the connect is done.
	assert(errno == EINPROGRESS);
	// Apparently, it is not writable yet.
	myTask->myEventsReady &= ~IO_EVENT_WRITE;
}

bool
AsyncConnect::onIOEvent()
{
	if ((myTask->myEventsReady & IO_EVENT_WRITE) == 0)
	{
		if (myTask->myState == IO_TASK_STATE_DELETING)
		{
			// Cancellation.
			myIsDone = true;
			myRes = -1;
			myCoro.resume();
			return true;
		}
		return false;
	}
	myIsDone = true;
	myRes = 0;
	myCoro.resume();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

IOTask::IOTask(
	IOCore &core,
	int fd)
	: myState(IO_TASK_STATE_NEW)
	, myFd(fd)
	, myIdx(-1)
	, myEventsReady(0)
	, myAsyncOp(nullptr)
	, myCore(core)
{
	LOG_DEBUG("IOTask create");
	theCount.fetch_add(1, std::memory_order_relaxed);
}

IOTask::~IOTask()
{
	LOG_DEBUG("IOTask destroy");
	theCount.fetch_sub(1, std::memory_order_relaxed);
	assert(myState == IO_TASK_STATE_DELETING);
	assert(myFd >= 0);
	assert(myAsyncOp == nullptr);
	int rc = ::close(myFd);
	assert(rc == 0);
}

void
IOTask::close()
{
	myCore.unsubscribe(this);
}

//////////////////////////////////////////////////////////////////////////////////////////

IOCore::IOCore()
	: myFd(epoll_create1(0))
{
	LOG_DEBUG("IOCore create");
	myIsStopped = false;
	// Eventfd is used to wakeup from epoll_wait() for handling non-kernel events. For
	// example, to let IOCore know, that there are new or deleting tasks to process.
	myEventFd = eventfd(0, EFD_NONBLOCK);
	myEventSub = subscribe(myEventFd);
}

IOCore::~IOCore()
{
	LOG_DEBUG("IOCore destroy");
	unsubscribe(myEventSub);
	myEventSub = nullptr;
	myEventFd = -1;
	processQueues();
	assert(myTasks.empty());
	assert(myQueue.empty());
	assert(myFd >= 0);
	int rc = close(myFd);
	assert(rc == 0);
}

void
IOCore::wakeup()
{
	uint64_t val = 1;
	ssize_t rc = write(myEventFd, &val, sizeof(val));
	assert(rc == sizeof(val));
}

IOTask *
IOCore::subscribe(
	int fd)
{
	std::unique_lock lock(myMutex);
	for (IOTask *s : myTasks)
		assert(s->myFd != fd);
	for (IOTask *s : myQueue)
		assert(s->myFd != fd);
	IOTask *s = new IOTask(*this, fd);
	myQueue.push_back(s);
	mySize.fetch_add(1, std::memory_order_relaxed);
	wakeup();
	return s;
}

void
IOCore::unsubscribe(
	IOTask *s)
{
	std::unique_lock lock(myMutex);
	for (IOTask *is : myQueue)
		assert(is != s);
	assert(myTasks[s->myIdx] == s);
	assert(s->myState == IO_TASK_STATE_WORKING);
	s->myState = IO_TASK_STATE_DELETING;
	myQueue.push_back(s);
	mySize.fetch_add(1, std::memory_order_relaxed);
	wakeup();
}

void
IOCore::roll()
{
	processQueues();
	epoll_event evs[theEpollBatchSize];
	int rc = epoll_wait(myFd, evs, theEpollBatchSize, -1);
	if (rc < 0 && errno == EINTR)
		return;
	assert(rc >= 0);
	LOG_THIS_DEBUG(IOCore, roll, rc << " events");
	for (int i = 0; i < rc; ++i)
	{
		epoll_event& ev = evs[i];
		IOTask *s = (IOTask *)ev.data.ptr;
		int mask = 0;
		if ((ev.events & EPOLLIN) != 0)
			mask |= IO_EVENT_READ;
		if ((ev.events & EPOLLOUT) != 0)
			mask |= IO_EVENT_WRITE;
		assert((ev.events & ~(EPOLLIN | EPOLLOUT)) == 0);
		assert(mask != 0);
		const char *eventStr = "[empty]";
		if ((mask & IO_EVENT_READ) && (mask & IO_EVENT_WRITE))
		{
			eventStr = "[read,write]";
		}
		else if (mask & IO_EVENT_READ)
		{
			eventStr = "[read]";
		}
		else if (mask & IO_EVENT_WRITE)
		{
			eventStr = "[write]";
		}
		LOG_THIS_DEBUG(IOCore, roll, "event " << i << ": " << eventStr);
		s->myEventsReady |= mask;
		AsyncOperation* op = s->myAsyncOp;
		if (op != nullptr)
		{
			// Nullify in case the coroutine would try to start a new async operation.
			s->myAsyncOp = nullptr;
			// Restore it back in case the handling didn't work. For example, due to a
			// spurious wakeup.
			if (!op->onIOEvent())
				s->myAsyncOp = op;
		}
	}
}

void
IOCore::processQueues()
{
	if (mySize.load(std::memory_order_relaxed) == 0)
		return;
	std::unique_lock lock(myMutex);
	if (myQueue.empty())
		return;
	for (IOTask *s : myQueue)
	{
		if (s->myState == IO_TASK_STATE_NEW)
		{
			LOG_THIS_DEBUG(IOCore, processQueues, "add " << s);
			s->myState = IO_TASK_STATE_WORKING;
			// Assume that in a new socket all the events are there. The task will clear
			// those which are not really available yet.
			s->myEventsReady = IO_EVENT_READ | IO_EVENT_WRITE;
			s->myIdx = myTasks.size();
			epoll_event ev;
			memset(&ev, 0, sizeof(ev));
			ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
			ev.data.ptr = (void *)s;
			int rc = epoll_ctl(myFd, EPOLL_CTL_ADD, s->myFd, &ev);
			assert(rc == 0);
			myTasks.push_back(s);
		}
		else if (s->myState == IO_TASK_STATE_DELETING)
		{
			assert(myTasks.size() > s->myIdx);
			assert(myTasks[s->myIdx] == s);
			assert(s->myFd >= 0);
			LOG_THIS_DEBUG(IOCore, processQueues, "drop " << s);
			// Cyclic deletion, for O(1).
			myTasks.back()->myIdx = s->myIdx;
			myTasks[s->myIdx] = myTasks.back();
			int rc = epoll_ctl(myFd, EPOLL_CTL_DEL, s->myFd, nullptr);
			assert(rc == 0);
			if (s->myAsyncOp != nullptr)
			{
				LOG_THIS_DEBUG(IOCore, processQueues, "cancel " << s);
				s->myEventsReady = 0;
				s->myAsyncOp->onIOEvent();
				s->myAsyncOp = nullptr;
			}
			delete s;
			myTasks.resize(myTasks.size() - 1);
		}
		else
		{
			assert(false);
		}
	}
	myQueue.clear();
	mySize.store(myTasks.size(), std::memory_order_relaxed);
}
