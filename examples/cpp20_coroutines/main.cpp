#include "iocoro.h"

#include <cassert>
#include <condition_variable>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <thread>
#include <unistd.h>

static constexpr uint64_t theRequestTargetCount = 50;
static constexpr int theClientCount = 100;

static uint64_t
getUsec();

static void
ioCoreRunF(
	IOCore &core);

static void
makeFdNonblock(
	int fd);

//////////////////////////////////////////////////////////////////////////////////////////

class Context
{
public:
	Context() : myFinishClientCount(0), myIsServerFinished(false) {}

	void
	onClientFinish();

	void
	onServerFinish();

	void
	waitClientsFinish();

	void
	waitServerFinish();

private:
	std::mutex myMutex;
	std::condition_variable myCond;
	uint64_t myFinishClientCount;
	bool myIsServerFinished;
};

//////////////////////////////////////////////////////////////////////////////////////////

struct Client
{
	Client(
		const std::shared_ptr<Context>& ctx);
	~Client();

	void
	connectAndRun(
		IOCore &core,
		uint16_t port);

	void
	wrapAndRun(
		IOCore &core,
		int sock);

	static std::atomic_int theCount;

private:
	IOCoroutine
	coroRun();

	IOTask *myTask;
	uint64_t myRecvCount;
	uint64_t mySendCount;
	const std::shared_ptr<Context> myContext;
};

std::atomic_int Client::theCount{0};

//////////////////////////////////////////////////////////////////////////////////////////

struct Server
{
	Server(
		const std::shared_ptr<Context>& ctx);
	~Server();

	uint16_t
	bindAndListenAndRun(
		IOCore &core);

	void
	stop();

private:
	IOCoroutine
	coroRun();

	IOTask *myTask;
	const std::shared_ptr<Context> myContext;
};

//////////////////////////////////////////////////////////////////////////////////////////

static int
run()
{
	std::shared_ptr<Context> context = std::make_shared<Context>();

	IOCore serverCore;
	std::cout << "start server" << std::endl;
	Server server(context);
	uint16_t port = server.bindAndListenAndRun(serverCore);
	std::thread serverThread(ioCoreRunF, std::ref(serverCore));

	std::cout << "start clients" << std::endl;
	IOCore clientCore;
	for (int i = 0; i < theClientCount; ++i)
		(new Client(context))->connectAndRun(clientCore, port);

	std::cout << "wait for the load to pass" << std::endl;
	uint64_t t1 = getUsec();
	std::thread clientThread(ioCoreRunF, std::ref(clientCore));
	context->waitClientsFinish();
	clientCore.stop();
	clientThread.join();
	uint64_t t2 = getUsec();
	std::cout << "Took " << (t2 - t1) / 1000.0 << " ms" << std::endl;

	std::cout << "wait for the server to stop" << std::endl;
	server.stop();
	context->waitServerFinish();
	serverCore.stop();
	serverThread.join();
	return 0;
}

int main()
{
	int rc = run();
	assert(Client::theCount.load(std::memory_order_relaxed) == 0);
	assert(IOCoroutinePromise::theCount.load(std::memory_order_relaxed) == 0);
	assert(IOTask::theCount.load(std::memory_order_relaxed) == 0);
	return rc;
}

//////////////////////////////////////////////////////////////////////////////////////////

static uint64_t
getUsec()
{
	timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec * 1'000'000 + t.tv_nsec / 1000;
}

static void
ioCoreRunF(IOCore &core)
{
	while (!core.isStopped())
		core.roll();
}

static void
makeFdNonblock(
	int fd)
{
	int rc = fcntl(fd, F_GETFL, 0);
	assert(rc >= 0);
	rc = fcntl(fd, F_SETFL, rc | O_NONBLOCK);
	assert(rc == 0);
}

//////////////////////////////////////////////////////////////////////////////////////////

void
Context::onClientFinish()
{
	std::unique_lock lock(myMutex);
	++myFinishClientCount;
	myCond.notify_one();
}

void
Context::onServerFinish()
{
	std::unique_lock lock(myMutex);
	assert(!myIsServerFinished);
	myIsServerFinished = true;
	myCond.notify_one();
}

void
Context::waitClientsFinish()
{
	constexpr uint64_t target = theClientCount * 2;
	std::unique_lock lock(myMutex);
	while (myFinishClientCount < target)
		myCond.wait(lock);
	assert(myFinishClientCount == target);
}

void
Context::waitServerFinish()
{
	std::unique_lock lock(myMutex);
	while (!myIsServerFinished)
		myCond.wait(lock);
}

//////////////////////////////////////////////////////////////////////////////////////////

Client::Client(
	const std::shared_ptr<Context>& ctx)
	: myTask(nullptr)
	, myRecvCount(0)
	, mySendCount(0)
	, myContext(ctx)
{
	LOG_THIS_DEBUG(Client, Client, "create");
	theCount.fetch_add(1, std::memory_order_relaxed);
}

Client::~Client()
{
	LOG_THIS_DEBUG(Client, ~Client, "drop " << myTask);
	theCount.fetch_sub(1, std::memory_order_relaxed);
	myTask->close();
}

void
Client::connectAndRun(
	IOCore &core,
	uint16_t port)
{
	LOG_THIS_DEBUG(Client, connect, "");
	assert(myTask == nullptr);
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	assert(sock >= 0);
	makeFdNonblock(sock);
	myTask = core.subscribe(sock);

	// Lambda capture won't work with a coroutine. The coro also captures things but only
	// the arguments. It can't capture the lambda's captures alongside. This leads to the
	// weird crutch when the to-capture values have to be passed as parameters. Otherwise
	// the coroutine body would reference lambda's captures which would be deleted when
	// the lambda object is destroyed, and that would lead to use-after-free.
	[](Client* self, uint16_t port) -> IOCoroutine {
		LOG_OBJ_DEBUG(Client, self, coroConnectAndRun, "");
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		int rc = co_await self->myTask->asyncConnect((sockaddr *)&addr, sizeof(addr));
		assert(rc == 0);
		// Unfortunately, can't reuse the current coroutine for running the main loop.
		// Have to create a new one.
		self->coroRun();
		co_return;
	}(this, port);
}

void
Client::wrapAndRun(
	IOCore &core,
	int sock)
{
	LOG_THIS_DEBUG(Client, wrap, myTask);
	assert(myTask == nullptr);
	makeFdNonblock(sock);
	myTask = core.subscribe(sock);
	coroRun();
}

IOCoroutine
Client::coroRun()
{
	LOG_THIS_DEBUG(Client, coroRun, "");
	for (uint32_t i = 0; i < theRequestTargetCount; ++i)
	{
		uint8_t data;
		LOG_THIS_DEBUG(Client, coroRun, "send");
		ssize_t rc = co_await myTask->asyncSend(&data, 1);
		LOG_THIS_DEBUG(Client, coroRun, "sent " << rc);
		assert(rc == 1);
		LOG_THIS_DEBUG(Client, coroRun, "receive");
		rc = co_await myTask->asyncRecv(&data, 1);
		LOG_THIS_DEBUG(Client, coroRun, "received " << rc);
		assert(rc == 1);
	}
	LOG_THIS_DEBUG(Client, coroRun, "finish");
	myContext->onClientFinish();
	delete this;
	co_return;
}

//////////////////////////////////////////////////////////////////////////////////////////

Server::Server(
	const std::shared_ptr<Context>& ctx)
	: myTask(nullptr)
	, myContext(ctx)
{
}

Server::~Server()
{
	assert(myTask == nullptr);
}

uint16_t
Server::bindAndListenAndRun(
	IOCore &core)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	socklen_t len = sizeof(addr);

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	assert(sock >= 0);
	int value = 1;
	int rc = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
	assert(rc == 0);
	rc = bind(sock, (sockaddr *)&addr, len);
	assert(rc == 0);
	rc = listen(sock, SOMAXCONN);
	assert(rc == 0);
	makeFdNonblock(sock);
	myTask = core.subscribe(sock);
	LOG_THIS_DEBUG(Server, bindAndListen, myTask);

	rc = getsockname(sock, (sockaddr *)&addr, &len);
	assert(rc == 0);
	assert(addr.sin_family == AF_INET);
	coroRun();
	return ntohs(addr.sin_port);
}

void
Server::stop()
{
	assert(myTask != nullptr);
	myTask->close();
	myTask = nullptr;
}

IOCoroutine
Server::coroRun()
{
	IOTask *task = myTask;
	while (true)
	{
		LOG_THIS_DEBUG(Server, coroRun, "accept start");
		sockaddr_in addr;
		socklen_t len = sizeof(addr);
		int sock = co_await task->asyncAccept((sockaddr *)&addr, &len);
		// Could be cancel.
		if (sock < 0)
			break;
		LOG_THIS_DEBUG(Server, coroRun, "new client, " << sock);
		(new Client(myContext))->wrapAndRun(task->core(), sock);
	}
	myContext->onServerFinish();
	co_return;
}
