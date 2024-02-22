#include "chat_client.h"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/read.hpp>
#include <iostream>

class chat_client_app final
{
public:
	chat_client_app(
		std::string_view name,
		std::string_view endpoint);

	int
	run();

private:
	void
	priv_recv_next();

	void
	priv_read_next();

	void
	priv_on_connect(
		chat_errcode err);

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
	chat_client m_cli;

	boost::asio::posix::stream_descriptor m_input;
	char m_in_buf[CHAT_RECV_BUF_SIZE];

	int m_res;
};

chat_client_app::chat_client_app(
	std::string_view name,
	std::string_view endpoint)
	: m_strand(m_ioctx)
	, m_cli(m_ioctx, name)
	, m_input(m_ioctx, dup(STDIN_FILENO))
	, m_res(0)
{
	m_cli.connect_async(endpoint, boost::asio::bind_executor(m_strand,
		std::bind(&chat_client_app::priv_on_connect, this, std::placeholders::_1)));
}

int
chat_client_app::run()
{
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(m_ioctx.get_executor());
	m_ioctx.run();
	return m_res;
}

void
chat_client_app::priv_recv_next()
{
	assert(m_strand.running_in_this_thread());
	m_cli.recv_async(boost::asio::bind_executor(m_strand,
		std::bind(&chat_client_app::priv_on_recv, this, std::placeholders::_1,
			std::placeholders::_2)));
}

void
chat_client_app::priv_read_next()
{
	assert(m_strand.running_in_this_thread());
	boost::asio::async_read(m_input, boost::asio::buffer(m_in_buf, CHAT_RECV_BUF_SIZE),
		boost::asio::bind_executor(m_strand,
			std::bind(&chat_client_app::priv_on_input, this, std::placeholders::_1,
				std::placeholders::_2)));
}

void
chat_client_app::priv_on_connect(
	chat_errcode err)
{
	assert(m_strand.running_in_this_thread());
	if (err != CHAT_ERR_NONE) {
		std::cout << "Connect error: chat " << err << '\n';
		m_res = -1;
		m_ioctx.stop();
		return;
	}
	priv_recv_next();
	priv_read_next();
}

void
chat_client_app::priv_on_recv(
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
chat_client_app::priv_on_input(
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
	m_cli.feed_async(std::string_view(m_in_buf, size));
	priv_read_next();
}

int
main(int argc, char **argv)
{
	if (argc < 2) {
		std::cout << "Expected an address to connect to\n";
		return -1;
	}
	const char *endpoint = argv[1];
	const char *name = argc >= 3 ? argv[2] : "anon";
	chat_client_app app(name, endpoint);
	return app.run();
}
