#pragma once

#include "chat.h"

#include <functional>
#include <memory>

namespace boost { namespace asio { class io_context; } }

using chat_client_on_connect_f = std::function<void(chat_errcode err)>;
using chat_client_on_msg_f = std::function<void(chat_errcode err, std::unique_ptr<chat_message> msg)>;

class chat_client_peer;

class chat_client final
{
public:
	chat_client(boost::asio::io_context& ioCtx, std::string_view name);
	~chat_client();

	void
	connect_async(
		std::string_view endpoint,
		chat_client_on_connect_f&& cb);

	void
	recv_async(
		chat_client_on_msg_f&& c);

	void
	feed_async(
		std::string_view text);

private:
	const std::shared_ptr<chat_client_peer> m_conn;
};
