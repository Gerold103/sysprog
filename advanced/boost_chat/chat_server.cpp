#include "chat.h"
#include "chat_server.h"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <list>

enum chat_server_state
{
	CHAT_SERVER_STATE_NEW,
	CHAT_SERVER_STATE_LISTEN,
	CHAT_SERVER_STATE_STOPPED,
};

enum chat_server_peer_state
{
	CHAT_SERVER_PEER_STATE_CONNECTED,
	CHAT_SERVER_PEER_STATE_STOPPED,
};

class chat_server_peer final : public std::enable_shared_from_this<chat_server_peer>
{
public:
	chat_server_peer(
		boost::asio::ip::tcp::socket&& sock,
		std::shared_ptr<chat_server_ctx> server);
	~chat_server_peer();

	void
	start();

	void
	stop();

	void
	feed_async(
		std::string_view text);

private:
	void
	priv_in_strand_on_new_feed(
		std::string&& text);

	void
	priv_in_strand_recv();

	void
	priv_in_strand_on_recv(
		const boost::system::error_code& err,
		std::size_t size);

	void
	priv_in_strand_send();

	void
	priv_in_strand_on_send(
		const boost::system::error_code& err,
		std::size_t size);

	void
	priv_in_strand_stop();

	chat_server_peer_state m_state;

	boost::asio::io_context::strand m_strand;
	boost::asio::ip::tcp::socket m_sock;
	std::shared_ptr<chat_server_ctx> m_server;

	std::string m_in_buf;
	std::string m_out_buf;

	// <YOUR CODE IF NEEDED>

	friend chat_server_ctx;
};

//////////////////////////////////////////////////////////////////////////////////////////

struct chat_server_request final
{
	chat_server_request(
		chat_server_on_msg_f&& cb) : m_cb(std::move(cb)) {}

	chat_server_on_msg_f m_cb;
};

//////////////////////////////////////////////////////////////////////////////////////////

class chat_server_ctx final : public std::enable_shared_from_this<chat_server_ctx>
{
public:
	chat_server_ctx(
		boost::asio::io_context& ioCtx);
	~chat_server_ctx();

	chat_errcode
	start(
		uint16_t port);

	uint16_t
	port() const;

	void
	stop();

	void
	recv_async(
		chat_server_on_msg_f&& cb);

	void
	feed_async(
		std::string_view text);

private:
	void
	priv_in_strand_accept();

	void
	priv_in_strand_on_accept(
		const boost::system::error_code& err,
		boost::asio::ip::tcp::socket sock);

	void
	priv_in_strand_stop();

	void
	priv_in_strand_on_new_request(
		std::unique_ptr<chat_server_request> req);

	void
	priv_peer_on_recv(
		std::unique_ptr<chat_message> msg);

	void
	priv_in_strand_peer_on_recv(
		std::unique_ptr<chat_message> msg);

	void
	priv_peer_on_close(
		std::shared_ptr<chat_server_peer> peer);

	void
	priv_in_strand_peer_on_close(
		std::shared_ptr<chat_server_peer> peer);

	void
	priv_in_strand_on_new_feed(
		std::string_view text);

	chat_server_state m_state;

	boost::asio::io_context::strand m_strand;
	boost::asio::ip::tcp::acceptor m_sock;
	uint16_t m_port;

	std::list<std::shared_ptr<chat_server_peer>> m_peers;

	std::list<std::unique_ptr<chat_server_request>> m_reqs;
	std::list<std::unique_ptr<chat_message>> m_in_msgs;

	// <YOUR CODE IF NEEDED>

	friend chat_server_peer;
};

//////////////////////////////////////////////////////////////////////////////////////////

chat_server::chat_server(
	boost::asio::io_context& ioCtx)
	: m_ctx(std::make_shared<chat_server_ctx>(ioCtx))
{
	// <YOUR CODE IF NEEDED>
}

chat_server::~chat_server()
{
	m_ctx->stop();

	// <YOUR CODE IF NEEDED>
}

chat_errcode
chat_server::start(
	uint16_t port)
{
	return m_ctx->start(port);
}

uint16_t
chat_server::port() const
{
	return m_ctx->port();
}

void
chat_server::recv_async(
	chat_server_on_msg_f&& cb)
{
	m_ctx->recv_async(std::move(cb));
}

void
chat_server::feed_async(
	std::string_view text)
{
	m_ctx->feed_async(text);
}

//////////////////////////////////////////////////////////////////////////////////////////

chat_server_peer::chat_server_peer(
	boost::asio::ip::tcp::socket&& sock,
	std::shared_ptr<chat_server_ctx> server)
	: m_state(CHAT_SERVER_PEER_STATE_CONNECTED)
	, m_strand(sock.get_executor().context())
	, m_sock(std::move(sock))
	, m_server(std::move(server))
{
}

chat_server_peer::~chat_server_peer()
{
	// <YOUR CODE IF NEEDED>
}

void
chat_server_peer::start()
{
	boost::asio::post(m_strand, std::bind(&chat_server_peer::priv_in_strand_recv,
		shared_from_this()));
}

void
chat_server_peer::stop()
{
	boost::asio::post(m_strand, std::bind(&chat_server_peer::priv_in_strand_stop,
		shared_from_this()));
}

void
chat_server_peer::feed_async(
	std::string_view text)
{
	boost::asio::post(m_strand, [ref = shared_from_this(), this,
		text = std::string(text)]() mutable {
		priv_in_strand_on_new_feed(std::move(text));
	});
}

void
chat_server_peer::priv_in_strand_on_new_feed(
	std::string&& /* text */)
{
	assert(m_strand.running_in_this_thread());

	// <YOUR CODE IF NEEDED>
	abort();

	// 1) Append the data to the pending output buffer m_out_buf.
	// 2) priv_in_strand_send();
}

void
chat_server_peer::priv_in_strand_recv()
{
	assert(m_strand.running_in_this_thread());
	if (m_state == CHAT_SERVER_PEER_STATE_STOPPED)
		return;

	// <YOUR CODE IF NEEDED>
	abort();

	// 1) Ensure you have enough space in m_in_buf. Reserve at least CHAT_RECV_BUF_SIZE.
	// Grow it x2 when need more.
	//
	// 2) Receive more data.
	// uint8_t* buf = m_in_buf.data() + m_in_buf.length();
	// size_t size = m_in_buf.capacity() - m_in_buf.length();
	// m_sock.async_receive(boost::asio::buffer(buf, size),
	//	boost::asio::bind_executor(strand_,
	//		std::bind(&chat_server_peer::priv_in_strand_on_recv, shared_from_this(),
	//		std::placeholder::_1, std::placeholders::_2)));
}

void
chat_server_peer::priv_in_strand_on_recv(
	const boost::system::error_code& /* err */,
	std::size_t /* size */)
{
	assert(m_strand.running_in_this_thread());

	// <YOUR CODE IF NEEDED>
	abort();

	// 1) Start considering the newly received bytes.
	// m_in_buf.resize(m_in_buf.length() + size);
	//
	// 2) Parse the buffer trying to extract complete messages. Send each extracted one
	// to the server: m_server->priv_peer_on_recv(msg).
	//
	// 3) Keep receiving infinitely.
	//
	// In your code you might make m_in_buf more efficient.
}

void
chat_server_peer::priv_in_strand_send()
{
	assert(m_strand.running_in_this_thread());
	if (m_state == CHAT_SERVER_PEER_STATE_STOPPED)
		return;

	// <YOUR CODE IF NEEDED>
	abort();

	// If the buffer m_out_buf contains a complete message, then send it using
	// m_sock.async_send(boost::asio::buffer(m_out_buf.data(), m_out_buf.length()),
	//	boost::asio::bind_executor(m_strand,
	//		std::bind(&chat_server_peer::priv_in_strand_on_send, shared_from_this(),
	//			std::placeholders::_1, std::placeholders::_2)));
	//
	// You don't have to send whole messages. Could send the parts right away if your
	// protocol is fine with that. But you might need to send full ones if you encode them
	// anyhow. For example, prefix each message with its total size to make receipt on the
	// other end more optimal.
	//
	// When m_out_buf is needed (only if you are sending full messages), you should keep
	// it valid and unchanged until the sending is complete. Which means you might need a
	// second buffer where you would store newer messages while the older ones are being
	// sent.
}

void
chat_server_peer::priv_in_strand_on_send(
	const boost::system::error_code& /* err */,
	std::size_t /* size */)
{
	assert(m_strand.running_in_this_thread());

	// <YOUR CODE IF NEEDED>
	abort();

	// Be careful about partial writes. The sent size might be < m_out_buf.length(). Then
	// you must send the rest again. Keep sending as long as you have something to send.
}

void
chat_server_peer::priv_in_strand_stop()
{
	assert(m_strand.running_in_this_thread());
	if (m_state != CHAT_SERVER_PEER_STATE_CONNECTED)
		return;
	m_state = CHAT_SERVER_PEER_STATE_CONNECTED;
	m_sock.close();
	m_server->priv_peer_on_close(shared_from_this());
	m_server.reset();
}

//////////////////////////////////////////////////////////////////////////////////////////

chat_server_ctx::chat_server_ctx(
	boost::asio::io_context& ioCtx)
	: m_state(CHAT_SERVER_STATE_NEW)
	, m_strand(ioCtx)
	, m_sock(ioCtx)
{
	// <YOUR CODE IF NEEDED>
}

chat_server_ctx::~chat_server_ctx()
{
	// <YOUR CODE IF NEEDED>
}

chat_errcode
chat_server_ctx::start(
	uint16_t port)
{
	assert(m_state == CHAT_SERVER_STATE_NEW);
	// <YOUR CODE IF NEEDED>
	//
	// You need to handle errors without throwing exceptions and return them
	// gracefully.
	//
	boost::asio::ip::tcp::endpoint endpoint;
	endpoint.port(port);
	m_sock.bind(endpoint);
	m_port = m_sock.local_endpoint().port();
	m_sock.listen();
	m_state = CHAT_SERVER_STATE_LISTEN;
	boost::asio::post(m_strand, std::bind(
		&chat_server_ctx::priv_in_strand_accept, shared_from_this()));
	return CHAT_ERR_NONE;
}

uint16_t
chat_server_ctx::port() const
{
	assert(m_state != CHAT_SERVER_STATE_NEW);
	return m_port;
}

void
chat_server_ctx::stop()
{
	boost::asio::post(m_strand, std::bind(
		&chat_server_ctx::priv_in_strand_stop, shared_from_this()));
}

void
chat_server_ctx::recv_async(
	chat_server_on_msg_f&& cb)
{
	std::unique_ptr<chat_server_request> req =
		std::make_unique<chat_server_request>(std::move(cb));
	boost::asio::post(m_strand,
		[ref = shared_from_this(), req = std::move(req), this]() mutable {
		priv_in_strand_on_new_request(std::move(req));
	});
}

void
chat_server_ctx::feed_async(
	std::string_view text)
{
	boost::asio::post(m_strand, std::bind(&chat_server_ctx::priv_in_strand_on_new_feed,
		shared_from_this(), std::string(text)));
}

void
chat_server_ctx::priv_in_strand_accept()
{
	assert(m_strand.running_in_this_thread());
	assert(m_state == CHAT_SERVER_STATE_LISTEN);
	m_sock.async_accept(boost::asio::bind_executor(m_strand, std::bind(
		&chat_server_ctx::priv_in_strand_on_accept, shared_from_this(),
		std::placeholders::_1, std::placeholders::_2)));
}

void
chat_server_ctx::priv_in_strand_on_accept(
	const boost::system::error_code& err,
	boost::asio::ip::tcp::socket sock)
{
	assert(m_strand.running_in_this_thread());
	if (m_state == CHAT_SERVER_STATE_STOPPED)
		return;
	if (err) {
		std::cout << "Chat server accept error: boost " << err << '\n';
		// There are no normal accept errors which could be handled greacefully. All of
		// them are critical.
		abort();
		return;
	}
	std::shared_ptr<chat_server_peer> peer = std::make_shared<chat_server_peer>(
		std::move(sock), shared_from_this());
	peer->start();
	m_peers.emplace_back(std::move(peer));
	priv_in_strand_accept();
}

void
chat_server_ctx::priv_in_strand_stop()
{
	assert(m_strand.running_in_this_thread());
	if (m_state != CHAT_SERVER_STATE_LISTEN)
		return;
	m_state = CHAT_SERVER_STATE_STOPPED;
	m_sock.close();
	for (std::shared_ptr<chat_server_peer>& p : m_peers)
		p->stop();
	m_peers.clear();
}

void
chat_server_ctx::priv_in_strand_on_new_request(
	std::unique_ptr<chat_server_request> req)
{
	assert(m_strand.running_in_this_thread());
	if (not m_reqs.empty() or m_in_msgs.empty()) {
		m_reqs.emplace_back(std::move(req));
		return;
	}
	// Already have data to return. Then just return it.
	req->m_cb(CHAT_ERR_NONE, std::move(m_in_msgs.front()));
	m_in_msgs.pop_front();
}

void
chat_server_ctx::priv_peer_on_recv(
	std::unique_ptr<chat_message> msg)
{
	boost::asio::post(m_strand, [ref = shared_from_this(), this,
		msg = std::move(msg)]() mutable {
		priv_in_strand_peer_on_recv(std::move(msg));
	});
}

void
chat_server_ctx::priv_in_strand_peer_on_recv(
	std::unique_ptr<chat_message> msg)
{
	assert(m_strand.running_in_this_thread());
	m_in_msgs.emplace_back(std::move(msg));

	// <YOUR CODE IF NEEDED>
	//
	// Serve the pending receive-requests if there are any.
}

void
chat_server_ctx::priv_peer_on_close(
	std::shared_ptr<chat_server_peer> peer)
{
	boost::asio::post(m_strand, [ref = shared_from_this(), this,
		peer = std::move(peer)]() mutable {
		priv_in_strand_peer_on_close(std::move(peer));
	});
}

void
chat_server_ctx::priv_in_strand_peer_on_close(
	std::shared_ptr<chat_server_peer> peer)
{
	assert(m_strand.running_in_this_thread());
	for (auto it = m_peers.begin(); it != m_peers.end(); ++it) {
		if (*it == peer) {
			m_peers.erase(it);
			return;
		}
	}
	if (m_state == CHAT_SERVER_STATE_STOPPED)
		return;
	// Unreachable. If it is reachable, then you have a bug.
	abort();
}

void
chat_server_ctx::priv_in_strand_on_new_feed(
	std::string_view text)
{
	assert(m_strand.running_in_this_thread());
	for (std::shared_ptr<chat_server_peer>& p : m_peers)
		p->feed_async(text);
}

//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
