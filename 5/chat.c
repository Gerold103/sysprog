#include "chat.h"

#include <poll.h>
#include <stdlib.h>

void
chat_message_delete(struct chat_message *msg)
{
	free(msg->data);
	free(msg);
}

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
