#include "libcoro.h"

#include "unit.h"

////////////////////////////////////////////////////////////////////////////////

static void *
my_coro_suspend_and_return_f(void *arg)
{
	printf("my_coro_suspend_and_return_f(): start\n");
	coro_suspend();
	printf("my_coro_suspend_and_return_f(): end\n");
	return arg;
}

static void *
my_coro_wakeup_another_f(void *arg)
{
	printf("my_coro_wakeup_another_f()\n");
	coro_wakeup(arg);
	return NULL;
}

static void
test_basic(void)
{
	unit_test_start();

	int data;
	struct coro *c1 = coro_new(my_coro_suspend_and_return_f, &data);
	struct coro *c2 = coro_new(my_coro_wakeup_another_f, c1);
	unit_check(coro_join(c2) == NULL, "coro wakeup worker result");
	unit_check(coro_join(c1) == &data, "coro suspect worker result");

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

// TODO: test order loop cyclic with yields.
// TODO: test own wakeup.
// TODO: test join of coro doing another join.
// TODO: test finished wakeup.

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
	printf("main(): start sched\n");
	coro_sched_init();
	printf("main(): start main coro\n");
	struct coro *main_coro = coro_new(coro_main_f, NULL);
	printf("main(): run sched\n");
	coro_sched_run();
	printf("main(): join main coro\n");
	void *rc = coro_join(main_coro);
	unit_check(rc == NULL, "main coro rc");
	printf("main(): destroy sched\n");
	coro_sched_destroy();
	return 0;
}
