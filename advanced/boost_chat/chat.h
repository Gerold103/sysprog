#pragma once

#include <condition_variable>
#include <mutex>
#include <string>

enum
{
	CHAT_RECV_BUF_SIZE = 128,
};

enum chat_errcode
{
	CHAT_ERR_NONE = 0,
	CHAT_ERR_INVALID_ARGUMENT,
	CHAT_ERR_TIMEOUT,
	CHAT_ERR_PORT_BUSY,
	CHAT_ERR_NO_ADDR,
	CHAT_ERR_ALREADY_STARTED,
	CHAT_ERR_NOT_IMPLEMENTED,
	CHAT_ERR_NOT_STARTED,
	CHAT_ERR_SYS,
	CHAT_ERR_CANCELED,
};

struct chat_message
{
	// Author's name.
	std::string m_author;
	std::string m_data;

	// <YOUR CODE IF NEEDED>
};

struct event
{
public:
	event();

	void
	send();
	void
	recv();

private:
	std::mutex m_mutex;
	std::condition_variable m_cond;
	bool m_is_set;
};