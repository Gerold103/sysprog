#include "libcoro.h"

#include "unit.h"

#include "corobus.h"

////////////////////////////////////////////////////////////////////////////////

static void
test_basic(void)
{
	unit_test_start();

	struct coro_bus *bus = coro_bus_new();
	coro_bus_delete(bus);

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void *
coro_main_f(void *arg)
{
	(void)arg;
	test_basic();
	return NULL;
}

int
main(void)
{
	coro_sched_init();
	struct coro *main_coro = coro_new(coro_main_f, NULL);
	coro_sched_run();
	void *rc = coro_join(main_coro);
	unit_check(rc == NULL, "main coro rc");
	coro_sched_destroy();
	return 0;
}
