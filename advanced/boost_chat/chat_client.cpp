#include "chat.h"
#include "chat_client.h"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <list>

struct chat_client_request final
{
	chat_client_request(
		chat_client_on_msg_f&& cb) : m_cb(std::move(cb)) {}

	chat_client_on_msg_f m_cb;
};

//////////////////////////////////////////////////////////////////////////////////////////

class chat_client_peer final : public std::enable_shared_from_this<chat_client_peer>
{
public:
	chat_client_peer(
		boost::asio::io_context& ioCtx,
		std::string_view name);
	~chat_client_peer();

	void
	connect_async(
		std::string_view endpoint,
		chat_client_on_connect_f&& cb);

	void
	recv_async(
		chat_client_on_msg_f&& cb);

	void
	feed_async(
		std::string_view text);

private:
	void
	priv_in_strand_on_resolve(
		const boost::system::error_code& err,
		chat_client_on_connect_f&& cb,
		boost::asio::ip::tcp::resolver::results_type results);

	void
	priv_in_strand_on_connect(
		const boost::system::error_code& err,
		chat_client_on_connect_f&& cb);

	void
	priv_in_strand_on_new_request(
		std::unique_ptr<chat_client_request> req);

	void
	priv_in_strand_recv();

	void
	priv_in_strand_on_recv(
		const boost::system::error_code& err,
		std::size_t size);

	void
	priv_in_strand_on_new_feed(
		std::string_view text);

	void
	priv_in_strand_send();

	void
	priv_in_strand_on_send(
		const boost::system::error_code& err,
		std::size_t size);

	// Strand "serializes" all callbacks associated with it. It means the strand will
	// invoke them one by one, never in more than one thread at a time. That in turn
	// means, that inside strand callbacks you don't need to protect its data with any
	// mutexes.
	boost::asio::io_context::strand m_strand;
	boost::asio::ip::tcp::socket m_sock;

	// Requests which are waiting for data.
	std::list<std::unique_ptr<chat_client_request>> m_reqs;
	// Full messages waiting to be delivered to requests.
	std::list<std::unique_ptr<chat_message>> m_in_msgs;
	// Input buffer for reading the next incoming messages.
	std::string m_in_buf;
	// Output buffer for prearing the next outgoing messages.
	std::string m_out_buf;

	boost::asio::ip::tcp::resolver m_resolver;
	const std::string m_name;

	// <YOUR CODE IF NEEDED>
};

//////////////////////////////////////////////////////////////////////////////////////////

chat_client::chat_client(
	boost::asio::io_context& ioCtx,
	std::string_view name)
	: m_conn(std::make_shared<chat_client_peer>(ioCtx, name))
{
	// <YOUR CODE IF NEEDED>
}

chat_client::~chat_client()
{
	// <YOUR CODE IF NEEDED>
}

void
chat_client::connect_async(
	std::string_view endpoint,
	chat_client_on_connect_f&& cb)
{
	m_conn->connect_async(endpoint, std::move(cb));
}

void
chat_client::recv_async(
	chat_client_on_msg_f&& cb)
{
	m_conn->recv_async(std::move(cb));
}

void
chat_client::feed_async(
	std::string_view text)
{
	m_conn->feed_async(text);
}

//////////////////////////////////////////////////////////////////////////////////////////

chat_client_peer::chat_client_peer(
	boost::asio::io_context& ioCtx,
	std::string_view name)
	: m_strand(ioCtx)
	, m_sock(ioCtx)
	, m_resolver(ioCtx)
	, m_name(name)
{
	// <YOUR CODE IF NEEDED>
}

chat_client_peer::~chat_client_peer()
{
	for (std::unique_ptr<chat_client_request>& r : m_reqs)
		r->m_cb(CHAT_ERR_CANCELED, {});
	m_reqs.clear();

	// <YOUR CODE IF NEEDED>
}

void
chat_client_peer::connect_async(
	std::string_view /* endpoint */,
	chat_client_on_connect_f&& /* cb */)
{
	// <YOUR CODE IF NEEDED>
	abort();

	// 1) Extract host and port from the endpoint string.
	//
	// 2) Start resolving them to an IP:
	// m_resolver.async_resolve(host, port, boost::asio::bind_executor(m_strand,
	//	[ref = shared_from_this(), this, cb = std::move(cb)](
	//		const boost::system::error_code& err,
	//		boost::asio::ip::tcp::resolver::results_type results) {
	//	priv_in_strand_on_resolve(err, std::move(cb), results);
	// }));
}

void
chat_client_peer::recv_async(
	chat_client_on_msg_f&& cb)
{
	std::unique_ptr<chat_client_request> req =
		std::make_unique<chat_client_request>(std::move(cb));
	boost::asio::post(m_strand,
		[ref = shared_from_this(), req = std::move(req), this]() mutable {
		priv_in_strand_on_new_request(std::move(req));
	});
}

void
chat_client_peer::feed_async(
	std::string_view text)
{
	boost::asio::post(m_strand, std::bind(
		&chat_client_peer::priv_in_strand_on_new_feed, shared_from_this(),
		std::string(text)));
}

void
chat_client_peer::priv_in_strand_on_resolve(
	const boost::system::error_code& /* err */,
	chat_client_on_connect_f&& /* cb */,
	boost::asio::ip::tcp::resolver::results_type /* results */)
{
	assert(m_strand.running_in_this_thread());

	// <YOUR CODE IF NEEDED>
	abort();

	// 1) Choose one of the results (it is an array).
	// 2) Do
	// m_sock.async_connect(endpoint, boost::asio::bind_executor(m_strand,
	//	[ref = shared_from_this(), this, cb = std::move(cb)](
	//		const boost::system::error_code& err) {
	//	priv_in_strand_on_connect(err, std::move(cb));
	// }));
}

void
chat_client_peer::priv_in_strand_on_connect(
	const boost::system::error_code& /* err */,
	chat_client_on_connect_f&& /* cb */)
{
	assert(m_strand.running_in_this_thread());

	// <YOUR CODE IF NEEDED>
	abort();
}

void
chat_client_peer::priv_in_strand_on_new_request(
	std::unique_ptr<chat_client_request> req)
{
	assert(m_strand.running_in_this_thread());
	if (not m_reqs.empty()) {
		// Older requests are not served yet. Means we need to wait for them to be served
		// to preserve the FIFO order.
		m_reqs.emplace_back(std::move(req));
		return;
	}
	if (not m_in_msgs.empty()) {
		// Already have data to return. Then just return it.
		req->m_cb(CHAT_ERR_NONE, std::move(m_in_msgs.front()));
		m_in_msgs.pop_front();
		return;
	}

	// <YOUR CODE IF NEEDED>
	abort();

	// No ready messages. It means you need to receive some new ones.
	//
	// 1) Save the request for serving it later.
	// m_reqs.emplace_back(std::move(req));
	//
	// 2) priv_in_strand_recv();
}

void
chat_client_peer::priv_in_strand_recv()
{
	assert(m_strand.running_in_this_thread());

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
	//		std::bind(&chat_client_peer::priv_in_strand_on_recv, shared_from_this(),
	//		std::placeholder::_1, std::placeholders::_2)));
}

void
chat_client_peer::priv_in_strand_on_recv(
	const boost::system::error_code& /* err */,
	std::size_t /* size */)
{
	assert(m_strand.running_in_this_thread());

	// <YOUR CODE IF NEEDED>
	abort();

	// 1) Start considering the newly received bytes.
	// m_in_buf.resize(m_in_buf.length() + size);
	//
	// 2) Parse the buffer trying to extract complete messages. Put each extracted one
	// into m_in_msgs.
	//
	// 3) Hand the received messages from m_in_msgs to pending requests in m_reqs.
	//
	// 4) Keep receiving until you get at least one full message.
	//
	// In your code you might make m_in_buf more efficient.
}

void
chat_client_peer::priv_in_strand_on_new_feed(
	std::string_view /* text */)
{
	assert(m_strand.running_in_this_thread());

	// <YOUR CODE IF NEEDED>
	abort();

	// 1) Append the data to the pending output buffer m_out_buf.
	// 2) priv_in_strand_send();
}

void
chat_client_peer::priv_in_strand_send()
{
	assert(m_strand.running_in_this_thread());

	// <YOUR CODE IF NEEDED>
	abort();

	// If the buffer m_out_buf contains a complete message, then send it using
	// m_sock.async_send(boost::asio::buffer(m_out_buf.data(), m_out_buf.length()),
	//	boost::asio::bind_executor(m_strand,
	//		std::bind(&chat_client_peer::priv_in_strand_on_send, shared_from_this(),
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
chat_client_peer::priv_in_strand_on_send(
	const boost::system::error_code& /* err */,
	std::size_t /* size */)
{
	assert(m_strand.running_in_this_thread());

	// <YOUR CODE IF NEEDED>
	abort();

	// Be careful about partial writes. The sent size might be < m_out_buf.length(). Then
	// you must send the rest again. Keep sending as long as you have something to send.
}
