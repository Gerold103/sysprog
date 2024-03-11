#pragma once

#include <atomic>
#include <coroutine>
#include <iostream>
#include <mutex>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

#define MAYBE_UNUSED(...) ((void)sizeof(1, ##__VA_ARGS__))

#define LOG_IMPL(...)                                                                     \
	do                                                                                    \
	{                                                                                     \
		std::stringstream ss;                                                             \
		ss << __VA_ARGS__ << std::endl;                                                   \
		std::cerr << ss.str();                                                            \
	} while (false)

#define LOG_NOP(...) MAYBE_UNUSED(std::cerr << __VA_ARGS__)

// Enable the logging using LOG_IMPL if needed.
#define LOG_DEBUG LOG_NOP
#define LOG_OBJ_DEBUG(name, obj, method, ...) LOG_NOP(#name "(" << obj << ")::" #method << ": " << __VA_ARGS__)
#define LOG_THIS_DEBUG(name, ...) LOG_OBJ_DEBUG(name, this, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////////////////

class IOCore;
class IOTask;

enum IOEventBit
{
	IO_EVENT_READ = 1,
	IO_EVENT_WRITE = 2,
};

enum IOTaskState
{
	IO_TASK_STATE_NEW,
	IO_TASK_STATE_WORKING,
	IO_TASK_STATE_DELETING,
};

//////////////////////////////////////////////////////////////////////////////////////////

struct IOCoroutinePromise;

// C++20 coroutine has to be inherited from std::coroutine_handle with a promise type
// defined as 'promise_type' inside of it. The promise is essentially the coroutine
// itself, its methods, data. Also implicitly the compiler will add here all the captured
// coroutine's local variables.
//
struct IOCoroutine : std::coroutine_handle<IOCoroutinePromise>
{
	using promise_type = IOCoroutinePromise;
};

// Automatic destructor for the coroutines which deletes them on co_return. It doesn't
// have to be like that, but this allows not to keep track of those coroutines anywhere if
// there are guarantees that they eventually reach the co_return.
//
struct IOCoroutineFinalizer
{
	bool
	await_ready() noexcept { return false; }

	void
	await_resume() noexcept {}

	// The standard guarantees, that before 'await_suspend()' the coroutine is fully
	// stopped. Can do anything with it now.
	void
	await_suspend(
		std::coroutine_handle<> coro) noexcept { coro.destroy(); }
};

struct IOCoroutinePromise
{
	IOCoroutinePromise()
	{
		LOG_DEBUG("IOCoroutinePromise create " << this);
		theCount.fetch_add(1, std::memory_order_relaxed);
	}
	~IOCoroutinePromise()
	{
		LOG_DEBUG("IOCoroutinePromise destroy " << this);
		theCount.fetch_sub(1, std::memory_order_relaxed);
	}

	IOCoroutine
	get_return_object() { return {IOCoroutine::from_promise(*this)}; }

	std::suspend_never
	initial_suspend() noexcept { return {}; }

	IOCoroutineFinalizer
	final_suspend() noexcept { return {}; }

	void
	return_void() {}

	void
	unhandled_exception() { abort(); }

	// Keep track of the promise count to ensure there are no memory leaks.
	static std::atomic_int theCount;
};

//////////////////////////////////////////////////////////////////////////////////////////

struct AsyncOperation
{
	AsyncOperation(
		IOTask *sub);
	AsyncOperation(
		const AsyncOperation&) = delete;
	AsyncOperation& operator=(
		const AsyncOperation&) = delete;

	// co_await takes an argument - an awaitable object. The object has to define certain
	// methods, including await_suspend(), invoked right after the coroutine has been
	// stopped.
	bool
	await_suspend(
		std::coroutine_handle<> coro);

private:
	virtual bool
	onIOEvent() = 0;

protected:
	IOTask *const myTask;
	std::coroutine_handle<> myCoro;

	friend IOCore;
};

//////////////////////////////////////////////////////////////////////////////////////////

struct AsyncRecv final : public AsyncOperation
{
	AsyncRecv(
		IOTask *sub,
		void *data,
		size_t size);
	AsyncRecv(
		const AsyncRecv&) = delete;
	AsyncRecv& operator=(
		const AsyncRecv&) = delete;

	// Is called by co_await before suspension just in case the suspension is not needed.
	bool
	await_ready() const noexcept { return myRes >= 0; }

	// What co_await will return.
	ssize_t
	await_resume() { return myRes; }

private:
	void
	execute();

	bool
	onIOEvent() final;

	void *const myData;
	const size_t mySize;
	ssize_t myRes;
};

//////////////////////////////////////////////////////////////////////////////////////////

struct AsyncSend final : public AsyncOperation
{
	AsyncSend(
		IOTask *sub,
		const void *data,
		size_t size);
	AsyncSend(
		const AsyncSend&) = delete;
	AsyncSend& operator=(
		const AsyncSend&) = delete;

	bool
	await_ready() const noexcept { return myRes >= 0; }

	ssize_t
	await_resume() { return myRes; }

private:
	void
	execute();

	bool
	onIOEvent() final;

	const void *const myData;
	const size_t mySize;
	ssize_t myRes;
};

//////////////////////////////////////////////////////////////////////////////////////////

struct AsyncAccept final : public AsyncOperation
{
	AsyncAccept(
		IOTask *sub,
		sockaddr *addr,
		socklen_t *size);
	AsyncAccept(
		const AsyncAccept&) = delete;
	AsyncAccept& operator=(
		const AsyncAccept&) = delete;

	bool
	await_ready() const noexcept { return myRes >= 0; }

	int
	await_resume() { return myRes; }

private:
	void
	execute();

	bool
	onIOEvent() final;

	sockaddr *const myAddr;
	socklen_t *const mySize;
	int myRes;
};

//////////////////////////////////////////////////////////////////////////////////////////

struct AsyncConnect final : public AsyncOperation
{
	AsyncConnect(
		IOTask *sub,
		const sockaddr *addr,
		socklen_t size);
	AsyncConnect(
		const AsyncConnect&) = delete;
	AsyncConnect& operator=(
		const AsyncConnect&) = delete;

	bool
	await_ready() const noexcept { return myIsDone; }

	int
	await_resume() { return myRes; }

private:
	bool
	onIOEvent() final;

	bool myIsDone;
	int myRes;
};

//////////////////////////////////////////////////////////////////////////////////////////

class IOTask
{
public:
	IOTask(
		IOCore &core,
		int fd);

	~IOTask();

	void
	close();

	IOCore&
	core() { return myCore; }

	//////////////////////////////////////////////
	// Those all are arguments for co_await.
	//
	AsyncRecv
	asyncRecv(void *data, size_t size) { return AsyncRecv(this, data, size); }

	AsyncSend
	asyncSend(const void *data, size_t size) { return AsyncSend(this, data, size); }

	AsyncAccept
	asyncAccept(sockaddr *addr, socklen_t *size) { return AsyncAccept(this, addr, size); }

	AsyncConnect
	asyncConnect(const sockaddr *addr, socklen_t size) { return AsyncConnect(this, addr, size); }
	//
	//////////////////////////////////////////////

	static std::atomic_int theCount;

private:
	IOTaskState myState;
	const int myFd;
	int myIdx;
	// Mask of events which are ready for consumption.
	int myEventsReady;
	// Currently waiting async operation blocked by a co_await. Coroutine can't be blocked
	// more than once at a time, which means the current operation can only be one.
	AsyncOperation* myAsyncOp;
	IOCore &myCore;

	friend AsyncAccept;
	friend AsyncConnect;
	friend AsyncOperation;
	friend AsyncRecv;
	friend AsyncSend;
	friend IOCore;
};

//////////////////////////////////////////////////////////////////////////////////////////

// Event loop + IO operations with C++ coroutine support.
//
struct IOCore
{
	IOCore();
	~IOCore();

	void
	wakeup();

	void
	stop() { myIsStopped.store(true, std::memory_order_relaxed); wakeup(); }

	bool
	isStopped() const { return myIsStopped.load(std::memory_order_relaxed); }

	// Create a new task for async operations on the given fd.
	IOTask *
	subscribe(
		int fd);

	// Destroy the task asynchronously. The memory will be freed, the task can't be used
	// anymore after unsubscription.
	void
	unsubscribe(
		IOTask *s);

	// Get all pending events from the kernel and handle them. Can only be done in one
	// thread at a time.
	void
	roll();

private:
	void
	processQueues();

	int myEventFd;
	IOTask *myEventSub;
	int myFd;
	std::atomic_bool myIsStopped;

	std::mutex myMutex;
	// Tasks currently in work.
	std::vector<IOTask *> myTasks;
	// Incoming tasks. New and deleting ones.
	std::vector<IOTask *> myQueue;
	std::atomic_uint64_t mySize;
};
