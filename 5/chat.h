#pragma once

/**
 * Here you should specify which features do you want to implement via macros:
 * If you want to enable author name support, do:
 *
 *     #define NEED_AUTHOR 1
 *
 * To enable server-feed from admin do:
 *
 *     #define NEED_SERVER_FEED 1
 *
 * It is important to define these macros here, in the header, because it is
 * used by tests.
 */
#define NEED_AUTHOR 0
#define NEED_SERVER_FEED 0

#include <string>

enum chat_errcode {
	CHAT_ERR_INVALID_ARGUMENT = 1,
	CHAT_ERR_TIMEOUT,
	CHAT_ERR_PORT_BUSY,
	CHAT_ERR_NO_ADDR,
	CHAT_ERR_ALREADY_STARTED,
	CHAT_ERR_NOT_IMPLEMENTED,
	CHAT_ERR_NOT_STARTED,
	CHAT_ERR_SYS,
};

enum chat_events {
	CHAT_EVENT_INPUT = 1,
	CHAT_EVENT_OUTPUT = 2,
};

struct chat_message {
#if NEED_AUTHOR
	/** Author's name. */
	std::string author;
#endif
	/** 0-terminate text. */
	std::string data;

	/* PUT HERE OTHER MEMBERS */
};

/** Convert chat_events mask to events suitable for poll(). */
int
chat_events_to_poll_events(int mask);
