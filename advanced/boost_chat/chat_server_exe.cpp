#include "chat_server.h"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/read.hpp>
#include <iostream>

class chat_server_app final
{
public:
	chat_server_app(
		uint16_t port);

	int
	run();

private:
	void
	priv_recv_next();

	void
	priv_read_next();

	void
	priv_on_recv(
		chat_errcode err,
		std::unique_ptr<chat_message> msg);

	void
	priv_on_input(
		const boost::system::error_code& err,
		size_t size);

	boost::asio::io_context m_ioctx;
	boost::asio::io_context::strand m_strand;
	chat_server m_server;

	boost::asio::posix::stream_descriptor m_input;
	char m_in_buf[CHAT_RECV_BUF_SIZE];

	int m_res;
};

static int
port_from_str(const char *str, uint16_t *port)
{
	errno = 0;
	char *end = NULL;
	long res = strtol(str, &end, 10);
	if (res == 0 && errno != 0)
		return -1;
	if (*end != 0)
		return -1;
	if (res > UINT16_MAX || res < 0)
		return -1;
	*port = (uint16_t)res;
	return 0;
}

chat_server_app::chat_server_app(
	uint16_t port)
	: m_strand(m_ioctx)
	, m_server(m_ioctx)
	, m_input(m_ioctx, dup(STDIN_FILENO))
	, m_res(0)
{
	m_server.start(port);
	boost::asio::post(m_strand, std::bind(&chat_server_app::priv_recv_next, this));
	boost::asio::post(m_strand, std::bind(&chat_server_app::priv_read_next, this));
}

int
chat_server_app::run()
{
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(m_ioctx.get_executor());
	m_ioctx.run();
	return m_res;
}

void
chat_server_app::priv_recv_next()
{
	assert(m_strand.running_in_this_thread());
	m_server.recv_async(boost::asio::bind_executor(m_strand,
		std::bind(&chat_server_app::priv_on_recv, this, std::placeholders::_1,
			std::placeholders::_2)));
}

void
chat_server_app::priv_read_next()
{
	assert(m_strand.running_in_this_thread());
	boost::asio::async_read(m_input, boost::asio::buffer(m_in_buf, CHAT_RECV_BUF_SIZE),
		boost::asio::bind_executor(m_strand,
			std::bind(&chat_server_app::priv_on_input, this, std::placeholders::_1,
				std::placeholders::_2)));
}

void
chat_server_app::priv_on_recv(
	chat_errcode err,
	std::unique_ptr<chat_message> msg)
{
	assert(m_strand.running_in_this_thread());
	if (err != CHAT_ERR_NONE) {
		std::cout << "Recv error: chat " << err << '\n';
		m_res = -1;
		m_ioctx.stop();
		return;
	}
	std::cout << msg->m_author << ": " << msg->m_data;
	priv_recv_next();
}

void
chat_server_app::priv_on_input(
	const boost::system::error_code& err,
	size_t size)
{
	assert(m_strand.running_in_this_thread());
	if (err) {
		if (err == boost::asio::error::eof) {
			std::cout << "Input EOF, terminating\n";
		} else {
			std::cout << "Input error: boost " << err << '\n';
			m_res = -1;
		}
		m_ioctx.stop();
		return;
	}
	assert(size > 0);
	m_server.feed_async(std::string_view(m_in_buf, size));
	priv_read_next();
}

int
main(int argc, char **argv)
{
	if (argc < 2) {
		std::cout << "Expected a port to listen on\n";
		return -1;
	}
	uint16_t port = 0;
	int rc = port_from_str(argv[1], &port);
	if (rc != 0) {
		std::cout << "Invalid port\n";
		return -1;
	}
	chat_server_app app(port);
	return app.run();
}
