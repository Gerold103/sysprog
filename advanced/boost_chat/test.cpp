#include "chat.h"
#include "chat_client.h"
#include "chat_server.h"
#include "unitpp.h"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <thread>

enum
{
	TEST_MSG_ID_LEN = 64,
};

class io_core final
{
public:
	~io_core() { stop(); }

	void
	start(
		uint32_t thread_count)
	{
		assert(m_workers.empty());
		m_backend.restart();
		for (uint32_t i = 0; i < thread_count; ++i)
		{
			m_workers.push_back(std::make_unique<std::thread>(std::bind(
				&io_core::priv_worker_f, this)));
		}
	}

	void
	stop()
	{
		m_backend.stop();
		for (std::unique_ptr<std::thread>& w : m_workers)
			w->join();
		m_workers.clear();
	}

	boost::asio::io_context& backend() { return m_backend; }

private:
	void
	priv_worker_f()
	{
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(
			m_backend.get_executor());
		m_backend.run();
	}

	boost::asio::io_context m_backend;
	std::vector<std::unique_ptr<std::thread>> m_workers;
};

//////////////////////////////////////////////////////////////////////////////////////////

struct test_msg final
{
	void
	create(
		size_t len)
	{
		len += TEST_MSG_ID_LEN + 1;
		m_data.resize(len);
		memset(m_data.data(), '0', TEST_MSG_ID_LEN);
		for (size_t i = TEST_MSG_ID_LEN; i < len; ++i)
			m_data[i] = 'a' + i % ('z' - 'a' + 1);
		m_data[len] = '\n';
	}

	void
	set_id(
		uint32_t cli_id,
		uint32_t msg_id)
	{
		unit_assert(m_data.length() >= TEST_MSG_ID_LEN);
		memset(m_data.data(), '0', TEST_MSG_ID_LEN);
		int rc = sprintf(m_data.data(), "cli_%u_msg_%u ", cli_id, msg_id);
		// Ignore terminating zero.
		m_data[rc] = '0';
	}

	void
	clear_id()
	{
		unit_assert(m_data.length() >= TEST_MSG_ID_LEN);
		memset(m_data.data(), '0', TEST_MSG_ID_LEN);
	}

	void
	check_data(
		std::string_view data)
	{
		// Should at least have '\n'.
		unit_assert(not m_data.empty());
		unit_assert(data.length() == m_data.length() - 1);
		unit_assert(data.length() >= TEST_MSG_ID_LEN);
		unit_assert(memcmp(m_data.data(), data.data(), data.length()) == 0);
	}

	std::string m_data;
};

//////////////////////////////////////////////////////////////////////////////////////////

static void
chat_message_extract_id(
	chat_message& msg,
	uint32_t* cli_id,
	uint32_t* msg_id)
{
	unit_assert(msg.m_data.length() >= TEST_MSG_ID_LEN);
	int rc = sscanf(msg.m_data.data(), "cli_%u_msg_%u ", cli_id, msg_id);
	unit_assert(rc == 2);
	memset(msg.m_data.data(), '0', TEST_MSG_ID_LEN);
}

static inline std::string
make_addr_str(
	uint16_t port)
{
	return "localhost:" + std::to_string(port);
}

static std::unique_ptr<chat_message>
server_recv_blocking(
	chat_server& server)
{
	event ev;
	chat_errcode err;
	std::unique_ptr<chat_message> msg;
	server.recv_async([&](chat_errcode err_res,
		std::unique_ptr<chat_message> msg_res) mutable {
		err = err_res;
		msg = std::move(msg_res);
		ev.send();
	});
	ev.recv();
	unit_assert(err == CHAT_ERR_NONE);
	return msg;
}

static std::unique_ptr<chat_message>
client_recv_blocking(
	chat_client& cli)
{
	event ev;
	chat_errcode err;
	std::unique_ptr<chat_message> msg;
	cli.recv_async([&](chat_errcode err_res,
		std::unique_ptr<chat_message> msg_res) mutable {
		err = err_res;
		msg = std::move(msg_res);
		ev.send();
	});
	ev.recv();
	unit_assert(err == CHAT_ERR_NONE);
	return msg;
}

static chat_errcode
client_connect_blocking(
	chat_client& cli,
	std::string_view endpoint)
{
	event ev;
	chat_errcode err;
	cli.connect_async(endpoint, [&](chat_errcode err_res) mutable {
		err = err_res;
		ev.send();
	});
	ev.recv();
	return err;
}

static void
test_trivial()
{
	unit_test_start();

	io_core core;
	core.start(1);
	//
	// Delete the server without listen.
	//
	{
		chat_server server(core.backend());
	}
	//
	// Delete the server right after listen.
	//
	{
		chat_server server(core.backend());
		chat_errcode rc = server.start(0);
		unit_check(rc == CHAT_ERR_NONE, "start");
	}
	//
	// Delete the client right away.
	//
	{
		chat_client cli(core.backend(), "c1");
	}
	//
	// Delete the client after connect.
	//
	{
		chat_server server(core.backend());
		unit_assert(server.start(0) == CHAT_ERR_NONE);

		chat_client cli(core.backend(), "c1");
		chat_errcode rc = client_connect_blocking(cli, make_addr_str(server.port()));
		unit_check(rc == CHAT_ERR_NONE, "connect");
	}
}

static void
test_basic()
{
	unit_test_start();

	io_core core;
	core.start(3);

	chat_server server(core.backend());
	unit_assert(server.start(0) == CHAT_ERR_NONE);
	std::string endpoint = make_addr_str(server.port());
	//
	// Connect a client and send a message.
	//
	chat_client cli(core.backend(), "c1");
	unit_assert(client_connect_blocking(
		cli, make_addr_str(server.port())) == CHAT_ERR_NONE);

	cli.feed_async("hello\n");
	unit_msg("send message");
	std::unique_ptr<chat_message> msg = server_recv_blocking(server);
	unit_check(msg, "got msg");
	unit_check(msg->m_data == "hello", "data");
	unit_check(msg->m_author == "c1", "author");
	//
	// Send a non-zero terminated message.
	//
	// Yes, 5, not 10. Send only "msg1\n".
	unit_msg("send message");
	cli.feed_async(std::string_view("msg1\nmsg2\n", 5));
	msg = server_recv_blocking(server);
	unit_check(msg, "got msg");
	unit_check(msg->m_data == "msg1", "data");
	unit_check(msg->m_author == "c1", "author");
}

static void
test_big_messages()
{
	unit_test_start();

	io_core core;
	core.start(3);

	chat_server server(core.backend());
	unit_assert(server.start(0) == CHAT_ERR_NONE);
	std::string endpoint = make_addr_str(server.port());

	chat_client cli(core.backend(), "c1");
	unit_assert(client_connect_blocking(
		cli, make_addr_str(server.port())) == CHAT_ERR_NONE);

	uint32_t len = 1024 * 1024;
	test_msg req;
	req.create(len);

	uint32_t count = 100;
	uint32_t real_count = 0;
	event ev;

	chat_server_on_msg_f server_on_msg = [&](
		chat_errcode err,
		std::unique_ptr<chat_message> msg) {

		unit_assert(err == CHAT_ERR_NONE);
		req.check_data(msg->m_data);
		server.recv_async(chat_server_on_msg_f{server_on_msg});

		++real_count;
		if (real_count == count)
			ev.send();
	};
	server.recv_async(chat_server_on_msg_f{server_on_msg});

	for (uint32_t i = 0; i < count; ++i)
		cli.feed_async(req.m_data);

	ev.recv();
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	core.stop();
}

static void
test_multi_feed()
{
	unit_test_start();

	io_core core;
	core.start(3);

	chat_server server(core.backend());
	unit_assert(server.start(0) == CHAT_ERR_NONE);
	std::string endpoint = make_addr_str(server.port());

	chat_client cli(core.backend(), "c1");
	unit_assert(client_connect_blocking(
		cli, make_addr_str(server.port())) == CHAT_ERR_NONE);

	// Multiple messages in one string still should be split by '\n'. If
	// there is no '\n' in the end, then data must be accumulated.
	unit_msg("feed an incomplete batch");
	cli.feed_async("msg1\nmsg2\nmsg");

	std::unique_ptr<chat_message> msg = server_recv_blocking(server);
	unit_check(msg->m_data == "msg1", "msg1 data");
	unit_check(msg->m_author == "c1", "msg1 author");

	msg = server_recv_blocking(server);
	unit_check(msg->m_data == "msg2", "msg2 data");
	unit_check(msg->m_author == "c1", "msg2 author");

	event ev;
	std::atomic_bool is_received(false);
	msg.reset();
	server.recv_async([&](
		chat_errcode err,
		std::unique_ptr<chat_message> msg_res) {

		unit_assert(err == CHAT_ERR_NONE);
		msg = std::move(msg_res);
		is_received.store(true, std::memory_order_relaxed);
		ev.send();
	});
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	unit_check(not is_received.load(std::memory_order_relaxed), "no msg3 yet");

	unit_msg("feed the next part");
	cli.feed_async("345");
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	unit_check(not is_received.load(std::memory_order_relaxed), "still no msg3");

	unit_msg("feed the last part");
	cli.feed_async("678\n");
	ev.recv();
	unit_check(msg->m_data == "msg345678", "msg3 data");
	unit_check(msg->m_author =="c1", "msg3 author");
}

static void
test_multi_client(void)
{
	unit_test_start();

	io_core core;
	core.start(3);

	chat_server server(core.backend());
	unit_assert(server.start(0) == CHAT_ERR_NONE);
	std::string endpoint = make_addr_str(server.port());

	uint32_t client_count = 20;
	uint32_t msg_count = 100;
	uint32_t len = 1024;
	test_msg req;
	req.create(len);

	unit_msg("Connect clients");
	std::vector<std::unique_ptr<chat_client>> clis;
	clis.resize(client_count);
	for (uint32_t i = 0; i < client_count; ++i) {
		clis.emplace_back(std::make_unique<chat_client>(
			core.backend(), "cli_" + std::to_string(i)));
		unit_assert(client_connect_blocking(*clis.back(), endpoint) == CHAT_ERR_NONE);
	}

	unit_msg("Say hello");
	chat_client* cli = clis[client_count - 1].get();
	cli->feed_async("hello\n");
	std::unique_ptr<chat_message> rsp = server_recv_blocking(server);
	unit_assert(rsp->m_data == "hello");
	for (uint32_t i = 0; i < client_count - 1; ++i) {
		rsp = client_recv_blocking(*clis[i]);
		unit_assert(rsp->m_data == "hello");
	}

	unit_msg("Send messages");
	for (uint32_t mi = 0; mi < msg_count; ++mi) {
		for (uint32_t ci = 0; ci < client_count; ++ci) {
			req.set_id(ci, mi);
			clis[ci]->feed_async(req.m_data);
		}
	}

	unit_msg("Consume all messages");
	req.clear_id();
	std::vector<uint32_t> msg_counts;
	msg_counts.resize(client_count, 0);
	for (uint32_t i = 0, end = msg_count * client_count; i < end; ++i) {
		rsp = server_recv_blocking(server);
		uint32_t cli_id = 0;
		uint32_t msg_id = 0;
		chat_message_extract_id(*rsp, &cli_id, &msg_id);
		unit_assert(cli_id < client_count);
		unit_assert(msg_id < msg_count);
		unit_assert(msg_counts[cli_id] == msg_id);
		++msg_counts[cli_id];

		req.check_data(rsp->m_data);
		unit_assert(rsp->m_author == "cli_" + std::to_string(cli_id));
	}
	for (uint32_t ci = 0; ci < client_count; ++ci) {
		msg_counts.clear();
		msg_counts.resize(client_count, 0);
		cli = clis[ci].get();
		// -1 because own messages are not delivered to self.
		uint32_t total_msg_count = msg_count * (client_count - 1);
		for (uint32_t mi = 0; mi < total_msg_count; ++mi) {
			rsp = client_recv_blocking(*cli);
			uint32_t cli_id = 0;
			uint32_t msg_id = 0;
			chat_message_extract_id(*rsp, &cli_id, &msg_id);
			unit_assert(cli_id < client_count);
			unit_assert(msg_id < msg_count);
			unit_assert(msg_counts[cli_id] == msg_id);
			++msg_counts[cli_id];

			req.check_data(rsp->m_data);
			unit_assert(rsp->m_author == "cli_" + std::to_string(cli_id));
		}
		unit_assert(msg_counts[ci] == 0);
	}
}

struct test_stress_ctx final
{
	uint32_t msg_count;
	uint32_t msg_len;
	std::atomic_uint32_t thread_idx;
	uint16_t port;
	std::atomic_bool is_running;
};

static void
test_stress_worker_f(
	io_core& core,
	test_stress_ctx& ctx)
{
	uint32_t thread_idx = ctx.thread_idx.fetch_add(1, std::memory_order_relaxed);
	test_msg req;
	req.create(ctx.msg_len);

	chat_client cli(core.backend(), "cli_" + std::to_string(thread_idx));
	unit_assert(client_connect_blocking(cli, make_addr_str(ctx.port)) == CHAT_ERR_NONE);

	for (uint32_t i = 0; i < ctx.msg_count; ++i) {
		req.set_id(thread_idx, i);
		cli.feed_async(req.m_data);
	}
	while (ctx.is_running.load(std::memory_order_relaxed)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

static void
test_stress()
{
	unit_test_start();

	const uint32_t client_count = 10;
	io_core core;
	core.start(client_count);

	chat_server server(core.backend());
	unit_assert(server.start(0) == CHAT_ERR_NONE);
	std::string endpoint = make_addr_str(server.port());

	test_stress_ctx ctx;
	ctx.msg_count = 100;
	ctx.msg_len = 1024;
	ctx.thread_idx.store(0, std::memory_order_relaxed);
	ctx.port = server.port();
	ctx.is_running.store(true, std::memory_order_relaxed);

	unit_msg("Start client threads");
	std::vector<std::unique_ptr<std::thread>> threads;
	threads.resize(client_count);

	for (uint32_t i = 0; i < client_count; ++i) {
		threads[i] = std::make_unique<std::thread>(
			test_stress_worker_f, std::ref(core), std::ref(ctx));
	}
	std::vector<uint32_t> msg_counts;
	msg_counts.resize(client_count, 0);

	test_msg req;
	req.create(ctx.msg_len);

	unit_msg("Receive all messages");
	for (uint32_t i = 0, end = ctx.msg_count * client_count; i < end; ++i) {
		std::unique_ptr<chat_message> rsp = server_recv_blocking(server);
		uint32_t cli_id = 0;
		uint32_t msg_id = 0;
		chat_message_extract_id(*rsp, &cli_id, &msg_id);
		unit_assert(cli_id < client_count);
		unit_assert(msg_id < ctx.msg_count);
		unit_assert(msg_counts[cli_id] == msg_id);
		++msg_counts[cli_id];

		req.check_data(rsp->m_data);
		unit_assert(rsp->m_author == "cli_" + std::to_string(cli_id));
	}

	unit_msg("Clean up the clients");
	ctx.is_running.store(false, std::memory_order_relaxed);
	for (uint32_t i = 0; i < client_count; ++i)
		threads[i]->join();
}

static void
test_big_author()
{
	unit_test_start();

	uint64_t author1_len = 10 * 1024 * 1024;
	std::string author1;
	author1.resize(author1_len);
	memset(author1.data(), 'z', author1_len);

	uint64_t author2_len = 11 * 1024 * 1024;
	std::string author2;
	author2.resize(author2_len);
	memset(author2.data(), 'y', author2_len);

	io_core core;
	core.start(3);

	chat_server server(core.backend());
	unit_assert(server.start(0) == CHAT_ERR_NONE);
	std::string endpoint = make_addr_str(server.port());

	chat_client cli1(core.backend(), author1);
	unit_assert(client_connect_blocking(
		cli1, make_addr_str(server.port())) == CHAT_ERR_NONE);

	chat_client cli2(core.backend(), author2);
	unit_assert(client_connect_blocking(
		cli2, make_addr_str(server.port())) == CHAT_ERR_NONE);

	uint64_t body_len = 20 * 1024 * 1024;
	std::string body;
	body.resize(body_len + 1);
	memset(body.data(), 'm', body_len);
	body[body_len] = '\n';

	cli1.feed_async(body);
	std::unique_ptr<chat_message> rsp = server_recv_blocking(server);
	body[body_len] = 0;
	unit_check(rsp->m_data == body, "msg data");
	unit_check(rsp->m_author == author1, "msg author");

	rsp = client_recv_blocking(cli2);
	unit_check(rsp->m_data == body, "msg data");
	unit_check(rsp->m_author == author1, "msg author");
}

int
main(void)
{
	unit_test_start();

	test_trivial();
	test_basic();
	test_big_messages();
	test_multi_feed();
	test_multi_client();
	test_stress();
	test_big_author();
	return 0;
}
