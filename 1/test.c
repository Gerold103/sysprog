#include "libcoro.h"

#include "unit.h"

#include "corobus.h"

////////////////////////////////////////////////////////////////////////////////

struct ctx_send {
	struct coro_bus *bus;
	int channel;
	unsigned data;
	int rc;
	enum coro_bus_error_code err;
	bool is_started;
	bool is_done;
	struct coro *worker;
};

static void *
test_send_f(void *arg)
{
	struct ctx_send *ctx = arg;
	ctx->is_started = true;
	ctx->rc = coro_bus_send(ctx->bus, ctx->channel, ctx->data);
	ctx->err = coro_bus_errno();
	ctx->is_done = true;
	return NULL;
}

static void
test_send_start(struct ctx_send *ctx, struct coro_bus *bus, int channel, unsigned data)
{
	ctx->bus = bus;
	ctx->channel = channel;
	ctx->data = data;
	ctx->rc = -1;
	ctx->err = CORO_BUS_ERR_NONE;
	ctx->is_started = false;
	ctx->is_done = false;
	ctx->worker = coro_new(test_send_f, ctx);
}

static int
test_send_join(struct ctx_send *ctx)
{
	unit_assert(coro_join(ctx->worker) == NULL);
	unit_assert(ctx->is_done);
	coro_bus_errno_set(ctx->err);
	return ctx->rc;
}

////////////////////////////////////////////////////////////////////////////////

struct ctx_recv {
	struct coro_bus *bus;
	int channel;
	unsigned *data;
	int rc;
	enum coro_bus_error_code err;
	bool is_started;
	bool is_done;
	struct coro *worker;
};

static void *
test_recv_f(void *arg)
{
	struct ctx_recv *ctx = arg;
	ctx->is_started = true;
	ctx->rc = coro_bus_recv(ctx->bus, ctx->channel, ctx->data);
	ctx->err = coro_bus_errno();
	ctx->is_done = true;
	return NULL;
}

static void
test_recv_start(struct ctx_recv *ctx, struct coro_bus *bus, int channel, unsigned *data)
{
	ctx->bus = bus;
	ctx->channel = channel;
	ctx->data = data;
	ctx->rc = -1;
	ctx->err = CORO_BUS_ERR_NONE;
	ctx->is_started = false;
	ctx->is_done = false;
	ctx->worker = coro_new(test_recv_f, ctx);
}

static int
test_recv_join(struct ctx_recv *ctx)
{
	unit_assert(coro_join(ctx->worker) == NULL);
	unit_assert(ctx->is_done);
	coro_bus_errno_set(ctx->err);
	return ctx->rc;
}

////////////////////////////////////////////////////////////////////////////////

static void
test_basic(void)
{
	unit_test_start();

	struct coro_bus *bus = coro_bus_new();
	int c1 = coro_bus_channel_open(bus, 5);
	unit_check(c1 >= 0, "channel is open");

	unit_check(coro_bus_send(bus, c1, 123) == 0, "send");
	unsigned data = 0;
	unit_check(coro_bus_recv(bus, c1, &data) == 0, "recv");
	unit_check(data == 123, "result");

	coro_bus_channel_close(bus, c1);
	coro_bus_delete(bus);

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_channel_reopen(void)
{
	unit_test_start();

	struct coro_bus *bus = coro_bus_new();

	int c1 = coro_bus_channel_open(bus, 2);
	unit_check(c1 >= 0, "channel is open");
	unit_check(coro_bus_send(bus, c1, 123) == 0, "send");
	unit_check(coro_bus_send(bus, c1, 456) == 0, "send");
	unit_check(coro_bus_try_send(bus, c1, 789) != 0, "no send");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK, "is full");
	coro_bus_channel_close(bus, c1);

	unsigned data = 321;
	unit_check(coro_bus_recv(bus, c1, &data) != 0, "no recv");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL, "already destroyed");
	unit_check(data == 321, "data is unchanged");

	int c2 = coro_bus_channel_open(bus, 3);
	unit_check(coro_bus_try_recv(bus, c2, &data) != 0, "no recv from new channel");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK, "is empty");
	unit_check(coro_bus_send(bus, c1, 123) == 0, "send");
	unit_check(coro_bus_send(bus, c1, 456) == 0, "send");
	unit_check(coro_bus_send(bus, c1, 789) == 0, "send more than in the old channel");
	coro_bus_channel_close(bus, c2);

	coro_bus_delete(bus);

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_multiple_channels(void)
{
	unit_test_start();

	struct coro_bus *bus = coro_bus_new();

	int c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	int c2 = coro_bus_channel_open(bus, 3);
	unit_assert(c2 >= 0);
	int c3 = coro_bus_channel_open(bus, 1);
	unit_assert(c3 >= 0);

	unit_check(coro_bus_send(bus, c1, 11) == 0, "send to 1");
	unit_check(coro_bus_send(bus, c2, 21) == 0, "send to 2");

	unit_check(coro_bus_send(bus, c3, 31) == 0, "send to 3");
	unit_check(coro_bus_try_send(bus, c3, 0) != 0, "no send to 3");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK, "is full");

	unit_check(coro_bus_send(bus, c1, 12) == 0, "send to 1");
	unit_check(coro_bus_try_send(bus, c1, 0) != 0, "no send to 1");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK, "is full");

	unit_check(coro_bus_send(bus, c2, 22) == 0, "send to 2");
	unit_check(coro_bus_send(bus, c2, 23) == 0, "send to 2");
	unit_check(coro_bus_try_send(bus, c1, 0) != 0, "no send to 2");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK, "is full");

	unsigned data = 0;
	unit_check(coro_bus_recv(bus, c3, &data) == 0, "recv from 3");
	unit_check(data == 31, "result");
	unit_check(coro_bus_try_recv(bus, c3, &data) != 0, "no recv from 3");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK, "is empty");
	coro_bus_channel_close(bus, c3);

	unit_check(coro_bus_recv(bus, c2, &data) == 0, "recv from 2");
	unit_check(data == 21, "result");
	unit_check(coro_bus_recv(bus, c1, &data) == 0, "recv from 1");
	unit_check(data == 11, "result");
	unit_check(coro_bus_recv(bus, c2, &data) == 0, "recv from 2");
	unit_check(data == 22, "result");
	unit_check(coro_bus_recv(bus, c1, &data) == 0, "recv from 1");
	unit_check(data == 12, "result");

	unit_check(coro_bus_try_recv(bus, c1, &data) != 0, "no recv from 1");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK, "is empty");
	coro_bus_channel_close(bus, c1);

	unit_check(coro_bus_recv(bus, c2, &data) == 0, "recv from 2");
	unit_check(data == 23, "result");
	unit_check(coro_bus_try_recv(bus, c2, &data) != 0, "no recv from 2");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK, "is empty");
	coro_bus_channel_close(bus, c2);

	coro_bus_delete(bus);

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_send_blocking(void)
{
	unit_test_start();

	struct coro_bus *bus = coro_bus_new();
	int c1 = coro_bus_channel_open(bus, 3);
	unit_check(c1 >= 0, "channel is open");

	for (unsigned i = 0; i < 3; ++i)
		unit_assert(coro_bus_send(bus, c1, i) == 0);

	struct ctx_send ctx;
	test_send_start(&ctx, bus, c1, 3);
	coro_yield();
	unit_check(ctx.is_started, "started coro for sending");
	unit_check(!ctx.is_done, "still not sent, blocked");
	coro_wakeup(ctx.worker);
	coro_yield();
	unit_check(!ctx.is_done, "still not sent after a spurious wakeup");

	unsigned data = 0;
	unit_check(coro_bus_recv(bus, c1, &data) == 0, "recv");
	unit_check(data == 0, "result");
	unit_check(test_send_join(&ctx) == 0, "send was successful");

	for (unsigned i = 1; i < 4; ++i) {
		unit_check(coro_bus_recv(bus, c1, &data) == 0, "recv next");
		unit_check(data == i, "result");
	}

	coro_bus_channel_close(bus, c1);
	coro_bus_delete(bus);

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_send_blocking_recv_many(void)
{
	unit_test_start();

	struct coro_bus *bus = coro_bus_new();
	const unsigned limit = 3;
	int c1 = coro_bus_channel_open(bus, limit);
	unit_check(c1 >= 0, "channel is open");

	for (unsigned i = 0; i < limit; ++i)
		unit_assert(coro_bus_send(bus, c1, i) == 0);

	const int coro_count = 10;
	struct ctx_send ctx[coro_count];
	unit_msg("start many coros");
	for (int i = 0; i < coro_count; ++i)
		test_send_start(&ctx[i], bus, c1, i + limit);

	unit_msg("ensure they are all running but not finished yet");
	coro_yield();
	for (int i = 0; i < coro_count; ++i) {
		unit_assert(ctx[i].is_started);
		unit_assert(!ctx[i].is_done);
	}

	unit_msg("receive all the messages");
	for (unsigned i = 0; i < limit + coro_count; ++i) {
		unsigned data = 0;
		unit_assert(coro_bus_recv(bus, c1, &data) == 0);
		unit_assert(data == i);
	}

	unit_msg("finalize all the senders");
	for (int i = 0; i < coro_count; ++i)
		unit_assert(test_send_join(&ctx[i]) == 0);

	coro_bus_channel_close(bus, c1);
	coro_bus_delete(bus);

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_send_basic(void)
{
	unit_test_start();

	struct coro_bus *bus = coro_bus_new();

	/* Channel never existed. */
	unit_check(coro_bus_send(bus, 0, 123) != 0, "fail to send");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL, "because no channel");

	unit_check(coro_bus_try_send(bus, 0, 123) != 0, "fail to try to send");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL, "because no channel");

	/* Channel did exist. */
	coro_bus_channel_close(bus, coro_bus_channel_open(bus, 3));
	unit_check(coro_bus_send(bus, 0, 123) != 0, "fail to send");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL, "because no channel");

	unit_check(coro_bus_try_send(bus, 0, 123) != 0, "fail to try to send");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL, "because no channel");

	/* Channel is full. */
	int c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	unit_check(coro_bus_send(bus, c1, 1) == 0, "sent");
	unit_check(coro_bus_send(bus, c1, 2) == 0, "sent");
	unit_check(coro_bus_try_send(bus, c1, 3) != 0, "failed to send");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK, "because channel is full");
	unsigned data = 123;
	unit_check(coro_bus_recv(bus, c1, &data) == 0, "recv");
	unit_check(data == 1, "result");
	unit_check(coro_bus_recv(bus, c1, &data) == 0, "recv");
	unit_check(data == 2, "result");
	coro_bus_channel_close(bus, c1);

	/* Same, but only 'try send'. */
	c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	unit_check(coro_bus_try_send(bus, c1, 1) == 0, "sent");
	unit_check(coro_bus_try_send(bus, c1, 2) == 0, "sent");
	unit_check(coro_bus_try_send(bus, c1, 3) != 0, "failed to send");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK, "because channel is full");
	unit_check(coro_bus_recv(bus, c1, &data) == 0, "recv");
	unit_check(data == 1, "result");
	unit_check(coro_bus_recv(bus, c1, &data) == 0, "recv");
	unit_check(data == 2, "result");
	coro_bus_channel_close(bus, c1);

	/* Try-send wakes up a waiting receiver. */
	c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	struct ctx_recv ctx;
	test_recv_start(&ctx, bus, c1, &data);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);
	unit_check(coro_bus_try_send(bus, c1, 1) == 0, "sent");
	unit_check(test_recv_join(&ctx) == 0, "received");
	unit_check(data == 1, "result");
	coro_bus_channel_close(bus, c1);

	coro_bus_delete(bus);

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_recv_blocking(void)
{
	unit_test_start();

	struct coro_bus *bus = coro_bus_new();
	int c1 = coro_bus_channel_open(bus, 3);
	unit_check(c1 >= 0, "channel is open");

	unsigned data = 0;
	struct ctx_recv ctx;
	test_recv_start(&ctx, bus, c1, &data);
	coro_yield();
	unit_check(ctx.is_started, "started coro for receiving");
	unit_check(!ctx.is_done, "still not received, blocked");
	coro_wakeup(ctx.worker);
	coro_yield();
	unit_check(!ctx.is_done, "still not received after a spurious wakeup");

	unit_check(coro_bus_send(bus, c1, 123) == 0, "sent");
	unit_check(test_recv_join(&ctx) == 0, "receive was successful");
	unit_check(data == 123, "result");

	coro_bus_channel_close(bus, c1);
	coro_bus_delete(bus);

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_recv_blocking_send_many(void)
{
	unit_test_start();

	struct coro_bus *bus = coro_bus_new();
	const unsigned limit = 3;
	int c1 = coro_bus_channel_open(bus, limit);
	unit_check(c1 >= 0, "channel is open");

	const unsigned coro_count = 15;
	unsigned datas[coro_count];
	struct ctx_recv ctx[coro_count];
	unit_msg("start many coros");
	for (unsigned i = 0; i < coro_count; ++i)
		test_recv_start(&ctx[i], bus, c1, &datas[i]);

	unit_msg("ensure they are all running but not finished yet");
	coro_yield();
	for (unsigned i = 0; i < coro_count; ++i) {
		unit_assert(ctx[i].is_started);
		unit_assert(!ctx[i].is_done);
	}

	unit_msg("send all the messages");
	for (unsigned i = 0; i < limit + coro_count; ++i)
		unit_assert(coro_bus_send(bus, c1, i) == 0);

	unit_msg("finalize all the receivers");
	for (unsigned i = 0; i < coro_count; ++i) {
		unit_assert(test_recv_join(&ctx[i]) == 0);
		unit_assert(datas[i] == i);
	}

	unit_msg("receive the rest");
	for (unsigned i = coro_count; i < limit + coro_count; ++i) {
		unsigned data = 0;
		unit_assert(coro_bus_recv(bus, c1, &data) == 0);
		unit_assert(data == i);
	}

	coro_bus_channel_close(bus, c1);
	coro_bus_delete(bus);

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_wakeup_on_close(void)
{
	unit_test_start();

	struct coro_bus *bus = coro_bus_new();

	int c1 = coro_bus_channel_open(bus, 3);
	unit_check(c1 >= 0, "channel is open");
	for (unsigned i = 0; i < 3; ++i)
		unit_assert(coro_bus_send(bus, c1, i) == 0);
	struct ctx_send send_ctx1;
	test_send_start(&send_ctx1, bus, c1, 3);
	struct ctx_send send_ctx2;
	test_send_start(&send_ctx2, bus, c1, 4);
	coro_yield();
	unit_check(send_ctx1.is_started && send_ctx2.is_started, "started coros for sending");
	unit_check(!send_ctx1.is_done && !send_ctx2.is_done, "still not sent, blocked");
	coro_bus_channel_close(bus, c1);
	unit_check(test_send_join(&send_ctx1) != 0, "first send failed");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL, "because channel is gone");
	unit_check(test_send_join(&send_ctx2) != 0, "second send failed");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL, "because channel is gone");

	/* Same for receive. */
	c1 = coro_bus_channel_open(bus, 3);
	unit_check(c1 >= 0, "channel is open");
	unsigned data1 = 987;
	struct ctx_recv recv_ctx1;
	test_recv_start(&recv_ctx1, bus, c1, &data1);
	unsigned data2 = 654;
	struct ctx_recv recv_ctx2;
	test_recv_start(&recv_ctx2, bus, c1, &data2);
	coro_yield();
	unit_check(recv_ctx1.is_started && recv_ctx2.is_started, "started coros for receiving");
	unit_check(!recv_ctx1.is_done && !recv_ctx2.is_done, "still not received, blocked");
	coro_bus_channel_close(bus, c1);
	unit_check(test_recv_join(&recv_ctx1) != 0, "first recv failed");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL, "because channel is gone");
	unit_check(data1 == 987, "data did not change");
	unit_check(test_recv_join(&recv_ctx2) != 0, "second recv failed");
	unit_check(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL, "because channel is gone");
	unit_check(data2 == 654, "data did not change");

	coro_bus_delete(bus);

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

// stress test for sending and receiving in many coroutines
// coro_bus_broadcast()
// coro_bus_try_broadcast()
// coro_bus_send_v()
// coro_bus_try_send_v()
// coro_bus_recv_v()
// coro_bus_try_recv_v()

////////////////////////////////////////////////////////////////////////////////

static void *
coro_main_f(void *arg)
{
	(void)arg;
	test_basic();
	test_channel_reopen();
	test_multiple_channels();

	test_send_blocking();
	test_send_blocking_recv_many();
	test_send_basic();

	test_recv_blocking();
	test_recv_blocking_send_many();

	test_wakeup_on_close();
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
