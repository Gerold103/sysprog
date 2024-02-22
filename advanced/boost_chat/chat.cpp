#include "chat.h"

event::event() : m_is_set(false) {}

void
event::send()
{
	std::unique_lock lock(m_mutex);
	m_is_set = true;
}

void
event::recv()
{
	std::unique_lock lock(m_mutex);
	while (not m_is_set)
		m_cond.wait(lock);
}
