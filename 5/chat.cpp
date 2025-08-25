#include "chat.h"

#include <poll.h>

int
chat_events_to_poll_events(int mask)
{
	int res = 0;
	if ((mask & CHAT_EVENT_INPUT) != 0)
		res |= POLLIN;
	if ((mask & CHAT_EVENT_OUTPUT) != 0)
		res |= POLLOUT;
	return res;
}
