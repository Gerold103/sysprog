#include "libcoro.h"

#include "unit.h"

////////////////////////////////////////////////////////////////////////////////

static void *
test_suspend_and_return_f(void *arg)
{
	coro_suspend();
	return arg;
}

static void *
test_wakeup_f(void *arg)
{
	coro_wakeup((struct coro *)arg);
	return NULL;
}

static void
test_suspend(void)
{
	unit_test_start();

	int data;
	struct coro *c1 = coro_new(test_suspend_and_return_f, &data);
	struct coro *c2 = coro_new(test_wakeup_f, c1);
	unit_check(coro_join(c2) == NULL, "coro wakeup worker result");
	unit_check(coro_join(c1) == &data, "coro suspect worker result");

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

struct test_loop_ctx {
	int id;
	int yield_count;
	int coro_count;
	int *next_id;
};

static void *
test_loop_coro_f(void *arg)
{
	struct test_loop_ctx *ctx = (decltype(ctx))arg;
	for (int i = 0; i < ctx->yield_count; ++i) {
		unit_assert(*ctx->next_id == ctx->id);
		*ctx->next_id = (*ctx->next_id + 1) % ctx->coro_count;
		coro_yield();
	}
	return NULL;
}

static void
test_loop_of_yields(void)
{
	unit_test_start();

	const int coro_count = 5;
	const int yield_count = 10;
	struct coro *coros[coro_count];
	struct test_loop_ctx contexts[coro_count];
	int next_id = 0;
	for (int i = 0; i < coro_count; ++i)
	{
		struct test_loop_ctx *ctx = &contexts[i];
		ctx->id = i;
		ctx->yield_count = yield_count;
		ctx->coro_count = coro_count;
		ctx->next_id = &next_id;
		coros[i] = coro_new(test_loop_coro_f, ctx);
	}
	for (int i = 0; i < coro_count; ++i)
		unit_assert(coro_join(coros[i]) == NULL);

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

struct test_wakeup_self_ctx {
	bool is_woken_up_externally;
	bool is_suspended;
};

static void *
test_wakeup_self_and_suspend_f(void *arg)
{
	struct test_wakeup_self_ctx *ctx = (decltype(ctx))arg;

	coro_wakeup(coro_this());
	unit_check(!ctx->is_suspended, "not suspended yet");
	unit_check(!ctx->is_woken_up_externally, "not woken up externally yet");

	ctx->is_suspended = true;
	coro_suspend();
	unit_check(ctx->is_suspended, "is awake from suspension");
	ctx->is_suspended = false;

	unit_check(ctx->is_woken_up_externally, "had to be woken up externally");
	return NULL;
}

static void
test_wakup_self(void)
{
	unit_test_start();

	struct test_wakeup_self_ctx ctx;
	ctx.is_woken_up_externally = false;
	ctx.is_suspended = false;
	struct coro *c = coro_new(test_wakeup_self_and_suspend_f, &ctx);
	coro_yield();
	unit_check(ctx.is_suspended, "the coro is suspended and waits");
	ctx.is_woken_up_externally = true;
	coro_wakeup(c);
	unit_check(coro_join(c) == NULL, "the coro is finished");

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void *
test_join_f(void *arg)
{
	return coro_join((struct coro *)arg);
}

static void
test_join_of_join(void)
{
	unit_test_start();

	int data;
	struct coro *c1 = coro_new(test_suspend_and_return_f, &data);
	struct coro *c2 = coro_new(test_join_f, c1);
	struct coro *c3 = coro_new(test_join_f, c2);
	coro_yield();
	coro_wakeup(c1);
	unit_check(coro_join(c3) == &data, "join chain");

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_wakeup_of_finished(void)
{
	unit_test_start();

	struct coro *c = coro_new(test_wakeup_f, coro_this());
	coro_suspend();
	coro_wakeup(c);
	unit_check(coro_join(c) == NULL, "joined a coro 'woken up' after ending");

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void *
coro_main_f(void *arg)
{
	(void)arg;
	test_suspend();
	test_loop_of_yields();
	test_wakup_self();
	test_join_of_join();
	test_wakeup_of_finished();
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
