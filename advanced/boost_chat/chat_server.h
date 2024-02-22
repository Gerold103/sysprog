#pragma once

#include "chat.h"

#include <functional>

namespace boost { namespace asio { class io_context; } }

class chat_server_ctx;

using chat_server_on_msg_f = std::function<void(chat_errcode err, std::unique_ptr<chat_message> msg)>;

class chat_server final
{
public:
	chat_server(
		boost::asio::io_context& ioCtx);
	~chat_server();

	chat_errcode
	start(
		uint16_t port);

	uint16_t
	port() const;

	void
	recv_async(
		chat_server_on_msg_f&& cb);

	void
	feed_async(
		std::string_view text);

private:
	const std::shared_ptr<chat_server_ctx> m_ctx;
};
