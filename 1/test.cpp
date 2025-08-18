#include "libcoro.h"

#include "unit.h"
#include "corobus.h"

#include <string.h>

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
send_f(void *arg)
{
	struct ctx_send *ctx = (decltype(ctx))arg;
	ctx->is_started = true;
	ctx->rc = coro_bus_send(ctx->bus, ctx->channel, ctx->data);
	ctx->err = coro_bus_errno();
	ctx->is_done = true;
	return NULL;
}

static void
send_start(struct ctx_send *ctx, struct coro_bus *bus, int channel,
	unsigned data)
{
	ctx->bus = bus;
	ctx->channel = channel;
	ctx->data = data;
	ctx->rc = -1;
	ctx->err = CORO_BUS_ERR_NONE;
	ctx->is_started = false;
	ctx->is_done = false;
	ctx->worker = coro_new(send_f, ctx);
}

static int
send_join(struct ctx_send *ctx)
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
recv_f(void *arg)
{
	struct ctx_recv *ctx = (decltype(ctx))arg;
	ctx->is_started = true;
	ctx->rc = coro_bus_recv(ctx->bus, ctx->channel, ctx->data);
	ctx->err = coro_bus_errno();
	ctx->is_done = true;
	return NULL;
}

static void
recv_start(struct ctx_recv *ctx, struct coro_bus *bus, int channel,
	unsigned *data)
{
	ctx->bus = bus;
	ctx->channel = channel;
	ctx->data = data;
	ctx->rc = -1;
	ctx->err = CORO_BUS_ERR_NONE;
	ctx->is_started = false;
	ctx->is_done = false;
	ctx->worker = coro_new(recv_f, ctx);
}

static int
recv_join(struct ctx_recv *ctx)
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

	unit_msg("open a channel, use it, and close");
	int c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	unit_assert(coro_bus_send(bus, c1, 123) == 0);
	unit_assert(coro_bus_send(bus, c1, 456) == 0);
	unit_assert(coro_bus_try_send(bus, c1, 789) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	unit_msg("can not use it anymore, deleted");
	unsigned data = 321;
	unit_assert(coro_bus_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(data == 321);

	unit_msg("open and use another channel");
	int c2 = coro_bus_channel_open(bus, 3);
	// The channel descriptors must be reused. To avoid infinite growth of
	// the channel array in the bus.
	unit_assert(c1 == c2);
	unit_assert(coro_bus_try_recv(bus, c2, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	unit_assert(coro_bus_send(bus, c2, 123) == 0);
	unit_assert(coro_bus_send(bus, c2, 456) == 0);
	unit_assert(coro_bus_send(bus, c2, 789) == 0);
	coro_bus_channel_close(bus, c2);

	unit_msg("open 2 channels");
	c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	c2 = coro_bus_channel_open(bus, 2);
	unit_assert(c2 >= 0);
	unit_msg("push some data");
	unit_assert(coro_bus_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_send(bus, c2, 2) == 0);
	unit_msg("close the first one");
	coro_bus_channel_close(bus, c1);
	unit_msg("open again");
	c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	unit_msg("read all");
	data = 0;
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	unit_assert(coro_bus_try_recv(bus, c2, &data) == 0);
	unit_assert(data == 2);
	coro_bus_channel_close(bus, c1);
	coro_bus_channel_close(bus, c2);

	unit_msg("open and close many times");
	c1 = coro_bus_channel_open(bus, 2);
	for (int i = 0; i < 100; ++i) {
		c2 = coro_bus_channel_open(bus, 2);
		unit_assert(c2 >= 0);
		coro_bus_channel_close(bus, c2);
	}
	coro_bus_channel_close(bus, c1);

	coro_bus_delete(bus);
	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_multiple_channels(void)
{
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();

	unit_msg("create channels");
	int c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	int c2 = coro_bus_channel_open(bus, 3);
	unit_assert(c2 >= 0);
	int c3 = coro_bus_channel_open(bus, 1);
	unit_assert(c3 >= 0);

	unit_msg("send some");
	unit_assert(coro_bus_send(bus, c1, 11) == 0);
	unit_assert(coro_bus_send(bus, c2, 21) == 0);

	unit_msg("c3 is full");
	unit_assert(coro_bus_send(bus, c3, 31) == 0);
	unit_assert(coro_bus_try_send(bus, c3, 0) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);

	unit_msg("c1 is full");
	unit_assert(coro_bus_send(bus, c1, 12) == 0);
	unit_assert(coro_bus_try_send(bus, c1, 0) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);

	unit_msg("c2 is full");
	unit_assert(coro_bus_send(bus, c2, 22) == 0);
	unit_assert(coro_bus_send(bus, c2, 23) == 0);
	unit_assert(coro_bus_try_send(bus, c2, 0) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);

	unit_msg("recv all from c3");
	unsigned data = 0;
	unit_assert(coro_bus_recv(bus, c3, &data) == 0 && data == 31);
	unit_assert(coro_bus_try_recv(bus, c3, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c3);

	unit_msg("recv some");
	unit_assert(coro_bus_recv(bus, c2, &data) == 0 && data == 21);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 11);
	unit_assert(coro_bus_recv(bus, c2, &data) == 0 && data == 22);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 12);

	unit_msg("c1 is empty");
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	unit_msg("c2 is empty");
	unit_assert(coro_bus_recv(bus, c2, &data) == 0 && data == 23);
	unit_assert(coro_bus_try_recv(bus, c2, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c2);

	coro_bus_delete(bus);
	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_send_basic(void)
{
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();

	unit_msg("channel never existed");
	unit_assert(coro_bus_send(bus, 0, 123) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(coro_bus_try_send(bus, 0, 123) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);

	unit_msg("channel did exist");
	coro_bus_channel_close(bus, coro_bus_channel_open(bus, 3));
	unit_assert(coro_bus_send(bus, 0, 123) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(coro_bus_try_send(bus, 0, 123) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);

	unit_msg("channel is full");
	int c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	unit_assert(coro_bus_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_send(bus, c1, 2) == 0);
	unit_assert(coro_bus_try_send(bus, c1, 3) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	unsigned data = 123;
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 1);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 2);
	coro_bus_channel_close(bus, c1);

	unit_msg("same but with try-send");
	c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	unit_assert(coro_bus_try_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_try_send(bus, c1, 2) == 0);
	unit_assert(coro_bus_try_send(bus, c1, 3) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 1);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 2);
	coro_bus_channel_close(bus, c1);

	unit_msg("try-send wakes up the waiting receiver");
	c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	struct ctx_recv ctx;
	recv_start(&ctx, bus, c1, &data);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);
	unit_assert(coro_bus_try_send(bus, c1, 1) == 0);
	unit_assert(recv_join(&ctx) == 0 && data == 1);
	coro_bus_channel_close(bus, c1);

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
	unit_assert(c1 >= 0);

	unit_msg("fill the channel");
	for (unsigned i = 0; i < 3; ++i)
		unit_assert(coro_bus_send(bus, c1, i) == 0);

	unit_msg("start a blocking send");
	struct ctx_send ctx;
	send_start(&ctx, bus, c1, 3);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);

	unit_msg("spurious wakeup");
	coro_wakeup(ctx.worker);
	coro_yield();
	unit_assert(!ctx.is_done);

	unit_msg("free some space");
	unsigned data = 0;
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 0);

	unit_msg("sending is done");
	unit_assert(send_join(&ctx) == 0);

	unit_msg("check the data");
	for (unsigned i = 1; i < 4; ++i)
		unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == i);

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
	unit_assert(c1 >= 0);

	unit_msg("fill the channel");
	for (unsigned i = 0; i < limit; ++i)
		unit_assert(coro_bus_send(bus, c1, i) == 0);

	unit_msg("start many coros");
	const int coro_count = 10;
	struct ctx_send ctx[coro_count];
	for (int i = 0; i < coro_count; ++i)
		send_start(&ctx[i], bus, c1, i + limit);

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
		unit_assert(send_join(&ctx[i]) == 0);

	coro_bus_channel_close(bus, c1);
	coro_bus_delete(bus);

	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_recv_basic(void)
{
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();

	unit_msg("channel never existed");
	unsigned data;
	unit_assert(coro_bus_recv(bus, 0, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(coro_bus_try_recv(bus, 0, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);

	unit_msg("channel did exist");
	coro_bus_channel_close(bus, coro_bus_channel_open(bus, 3));
	unit_assert(coro_bus_recv(bus, 0, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(coro_bus_try_recv(bus, 0, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);

	unit_msg("channel is empty");
	int c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	unit_msg("recv wakes up the waiting sender");
	c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	unit_assert(coro_bus_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_send(bus, c1, 2) == 0);
	struct ctx_send ctx;
	send_start(&ctx, bus, c1, 3);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);
	data = 123;
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 1);
	unit_assert(send_join(&ctx) == 0);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 2);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 3);
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	unit_msg("same but with try-recv");
	c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	unit_assert(coro_bus_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_send(bus, c1, 2) == 0);
	send_start(&ctx, bus, c1, 3);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);
	data = 123;
	unit_assert(coro_bus_try_recv(bus, c1, &data) == 0 && data == 1);
	unit_assert(send_join(&ctx) == 0);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 2);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 3);
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
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
	unit_assert(c1 >= 0);

	unit_msg("start recv");
	unsigned data = 0;
	struct ctx_recv ctx;
	recv_start(&ctx, bus, c1, &data);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);

	unit_msg("spurious wakeup");
	coro_wakeup(ctx.worker);
	coro_yield();
	unit_assert(!ctx.is_done);

	unit_msg("finish the recv");
	unit_assert(coro_bus_send(bus, c1, 123) == 0);
	unit_assert(recv_join(&ctx) == 0 && data == 123);

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
	unit_assert(c1 >= 0);

	unit_msg("start many coros");
	const unsigned coro_count = 15;
	unsigned datas[coro_count];
	struct ctx_recv ctx[coro_count];
	for (unsigned i = 0; i < coro_count; ++i)
		recv_start(&ctx[i], bus, c1, &datas[i]);

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
		unit_assert(recv_join(&ctx[i]) == 0);
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

struct ctx_stress_send {
	struct coro_bus *bus;
	int channel;
	unsigned next_data;
	unsigned last_data;
};

static void *
stress_send_f(void *arg)
{
	struct ctx_stress_send *ctx = (decltype(ctx))arg;
	while (ctx->next_data < ctx->last_data) {
		unit_assert(coro_bus_send(ctx->bus, ctx->channel,
			ctx->next_data++) == 0);
	}
	return NULL;
}

struct ctx_stress_recv {
	struct coro_bus *bus;
	int channel;
	unsigned *datas;
	int capacity;
	int size;
};

static void *
stress_recv_f(void *arg)
{
	struct ctx_stress_recv *ctx = (decltype(ctx))arg;
	while (ctx->size < ctx->capacity) {
		unit_assert(coro_bus_recv(ctx->bus, ctx->channel,
			&ctx->datas[ctx->size++]) == 0);
	}
	return NULL;
}

struct ctx_stress_send_recv {
	struct coro_bus *bus;
	struct coro *worker;
	unsigned data_start;
};

static void *
stress_send_recv_f(void *arg)
{
	struct ctx_stress_send_recv *ctx = (decltype(ctx))arg;

	const unsigned limit = 10;
	const unsigned data_count = 1234;
	int c1 = coro_bus_channel_open(ctx->bus, limit);
	unit_assert(c1 >= 0);

	unit_msg("start sender");
	struct ctx_stress_send ctx_send;
	ctx_send.bus = ctx->bus;
	ctx_send.channel = c1;
	ctx_send.next_data = ctx->data_start;
	ctx_send.last_data = ctx->data_start + data_count;
	struct coro *sender = coro_new(stress_send_f, &ctx_send);

	unit_msg("start receiver");
	unsigned datas[data_count];
	struct ctx_stress_recv ctx_recv;
	ctx_recv.bus = ctx->bus;
	ctx_recv.channel = c1;
	ctx_recv.datas = datas;
	ctx_recv.capacity = data_count;
	ctx_recv.size = 0;
	struct coro *receiver = coro_new(stress_recv_f, &ctx_recv);

	unit_msg("join the sender and receiver");
	unit_assert(coro_join(sender) == NULL);
	unit_assert(coro_join(receiver) == NULL);

	unit_msg("check data");
	for (unsigned i = 0; i < data_count; ++i)
		unit_assert(datas[i] == ctx->data_start + i);

	coro_bus_channel_close(ctx->bus, c1);
	return NULL;
}

static void
test_stress_send_recv_concurrent(void)
{
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();
	const unsigned worker_count = 5;
	struct ctx_stress_send_recv contexts[worker_count];
	unit_msg("start workers");
	for (unsigned i = 0; i < worker_count; ++i) {
		struct ctx_stress_send_recv *ctx = &contexts[i];
		ctx->bus = bus;
		ctx->worker = coro_new(stress_send_recv_f, ctx);
		ctx->data_start = i * 10;
	}
	unit_msg("join workers");
	for (unsigned i = 0; i < worker_count; ++i)
		unit_assert(coro_join(contexts[i].worker) == NULL);
	coro_bus_delete(bus);
	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_send_recv_very_many(void)
{
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();
	const unsigned limit = 500;
	int c1 = coro_bus_channel_open(bus, limit);
	unit_assert(c1 >= 0);

	unit_msg("start sender");
	const unsigned data_count = 100000;
	struct ctx_stress_send ctx;
	ctx.bus = bus;
	ctx.channel = c1;
	ctx.next_data = 0;
	ctx.last_data = data_count;
	struct coro *sender = coro_new(stress_send_f, &ctx);

	unit_msg("receive");
	for (unsigned i = 0; i < data_count; ++i) {
		unsigned data = 0;
		unit_assert(coro_bus_recv(bus, c1, &data) == 0);
		unit_assert(data == i);
	}
	unit_assert(coro_join(sender) == NULL);

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

	unit_msg("open and fill a channel");
	int c1 = coro_bus_channel_open(bus, 3);
	unit_assert(c1 >= 0);
	for (unsigned i = 0; i < 3; ++i)
		unit_assert(coro_bus_send(bus, c1, i) == 0);

	unit_msg("start senders");
	struct ctx_send send_ctx1;
	send_start(&send_ctx1, bus, c1, 3);
	struct ctx_send send_ctx2;
	send_start(&send_ctx2, bus, c1, 4);
	coro_yield();
	unit_assert(send_ctx1.is_started && send_ctx2.is_started);
	unit_assert(!send_ctx1.is_done && !send_ctx2.is_done);

	unit_msg("close the channel");
	coro_bus_channel_close(bus, c1);
	unit_assert(send_join(&send_ctx1) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(send_join(&send_ctx2) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);

	unit_msg("same for receive - open a channel");
	c1 = coro_bus_channel_open(bus, 3);
	unit_assert(c1 >= 0);

	unit_msg("start receivers");
	unsigned data1 = 987;
	struct ctx_recv recv_ctx1;
	recv_start(&recv_ctx1, bus, c1, &data1);
	unsigned data2 = 654;
	struct ctx_recv recv_ctx2;
	recv_start(&recv_ctx2, bus, c1, &data2);
	coro_yield();
	unit_assert(recv_ctx1.is_started && recv_ctx2.is_started);
	unit_assert(!recv_ctx1.is_done && !recv_ctx2.is_done);

	unit_msg("close the channel");
	coro_bus_channel_close(bus, c1);
	unit_assert(recv_join(&recv_ctx1) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(data1 == 987);
	unit_assert(recv_join(&recv_ctx2) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(data2 == 654);

	coro_bus_delete(bus);
	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

static void
test_close_non_empty_bus(void)
{
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();

	int c1 = coro_bus_channel_open(bus, 3);
	unit_assert(c1 >= 0);
	int c2 = coro_bus_channel_open(bus, 10);
	unit_assert(c2 >= 0);

	unit_assert(coro_bus_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_send(bus, c1, 2) == 0);
	unit_assert(coro_bus_send(bus, c1, 3) == 0);
	unit_assert(coro_bus_send(bus, c2, 1) == 0);
	unit_assert(coro_bus_send(bus, c2, 2) == 0);

	coro_bus_delete(bus);
	unit_test_finish();
}

////////////////////////////////////////////////////////////////////////////////

#if NEED_BROADCAST
struct ctx_broadcast {
	struct coro_bus *bus;
	unsigned data;
	int rc;
	enum coro_bus_error_code err;
	bool is_started;
	bool is_done;
	struct coro *worker;
};

static void *
broadcast_f(void *arg)
{
	struct ctx_broadcast *ctx = (decltype(ctx))arg;
	ctx->is_started = true;
	ctx->rc = coro_bus_broadcast(ctx->bus, ctx->data);
	ctx->err = coro_bus_errno();
	ctx->is_done = true;
	return NULL;
}

static void
broadcast_start(struct ctx_broadcast *ctx, struct coro_bus *bus, unsigned data)
{
	ctx->bus = bus;
	ctx->data = data;
	ctx->rc = -1;
	ctx->err = CORO_BUS_ERR_NONE;
	ctx->is_started = false;
	ctx->is_done = false;
	ctx->worker = coro_new(broadcast_f, ctx);
}

static int
broadcast_join(struct ctx_broadcast *ctx)
{
	unit_assert(coro_join(ctx->worker) == NULL);
	coro_bus_errno_set(ctx->err);
	return ctx->rc;
}
#endif

static void
test_broadcast_basic(void)
{
#if NEED_BROADCAST
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();

	unit_msg("no channels");
	unit_assert(coro_bus_broadcast(bus, 123) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(coro_bus_try_broadcast(bus, 123) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);

	unit_msg("a channel existed but was deleted");
	int c1 = coro_bus_channel_open(bus, 3);
	unit_assert(c1 >= 0);
	coro_bus_channel_close(bus, c1);
	unit_assert(coro_bus_broadcast(bus, 123) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(coro_bus_try_broadcast(bus, 123) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);

	unit_msg("broadcast with holes");
	c1 = coro_bus_channel_open(bus, 3);
	unit_assert(c1 >= 0);
	int ctmp = coro_bus_channel_open(bus, 123);
	unit_assert(ctmp >= 0);
	int c2 = coro_bus_channel_open(bus, 2);
	unit_assert(c2 >= 0);
	int c3 = coro_bus_channel_open(bus, 4);
	unit_assert(c3 >= 0);
	coro_bus_channel_close(bus, ctmp);
	unit_assert(coro_bus_broadcast(bus, 123) == 0);
	unit_assert(coro_bus_try_broadcast(bus, 456) == 0);
	unsigned data = 0;
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 123);
	unit_assert(coro_bus_recv(bus, c2, &data) == 0 && data == 123);
	unit_assert(coro_bus_recv(bus, c3, &data) == 0 && data == 123);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 456);
	unit_assert(coro_bus_recv(bus, c2, &data) == 0 && data == 456);
	unit_assert(coro_bus_recv(bus, c3, &data) == 0 && data == 456);
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	unit_assert(coro_bus_try_recv(bus, c2, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	unit_assert(coro_bus_try_recv(bus, c3, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);
	coro_bus_channel_close(bus, c2);
	coro_bus_channel_close(bus, c3);

	unit_msg("broadcast wakes the receivers up");
	c1 = coro_bus_channel_open(bus, 3);
	unit_assert(c1 >= 0);
	data = 0;
	struct ctx_recv ctx;
	recv_start(&ctx, bus, c1, &data);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);
	unit_assert(coro_bus_broadcast(bus, 123) == 0);
	unit_assert(recv_join(&ctx) == 0 && data == 123);

	unit_msg("same with try-broadcast");
	data = 0;
	recv_start(&ctx, bus, c1, &data);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);
	unit_assert(coro_bus_try_broadcast(bus, 123) == 0);
	unit_assert(recv_join(&ctx) == 0 && data == 123);
	coro_bus_channel_close(bus, c1);

	coro_bus_delete(bus);
	unit_test_finish();
#endif
}

////////////////////////////////////////////////////////////////////////////////

static void
test_broadcast_blocking_basic(void)
{
#if NEED_BROADCAST
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();

	unit_msg("create some channels full with data");
	int c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	int c2 = coro_bus_channel_open(bus, 3);
	unit_assert(c2 >= 0);
	int c3 = coro_bus_channel_open(bus, 1);
	unit_assert(c3 >= 0);
	int c4 = coro_bus_channel_open(bus, 1);
	unit_assert(c4 >= 0);

	unit_assert(coro_bus_send(bus, c1, 0) == 0);
	unit_assert(coro_bus_send(bus, c2, 1) == 0);
	unit_assert(coro_bus_send(bus, c2, 2) == 0);
	unit_assert(coro_bus_send(bus, c3, 3) == 0);

	unit_msg("start a broadcast");
	struct ctx_broadcast ctx;
	broadcast_start(&ctx, bus, 999);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);

	unit_msg("spurious wakeup");
	coro_wakeup(ctx.worker);
	coro_yield();
	unit_assert(!ctx.is_done);

	unit_msg("close one of the channels");
	coro_bus_channel_close(bus, c4);

	unit_msg("make one channel not full, and another - full");
	unit_assert(coro_bus_send(bus, c2, 4) == 0);
	unit_assert(coro_bus_try_send(bus, c2, 5) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	unsigned data = 0;
	unit_assert(coro_bus_recv(bus, c3, &data) == 0 && data == 3);

	unit_msg("another spurious wakeup");
	coro_yield();
	unit_assert(!ctx.is_done);

	unit_msg("nothing is delivered yet");
	// Old message.
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 0);
	// No new messages yet.
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);

	unit_msg("finish the broadcast");
	unit_assert(coro_bus_recv(bus, c2, &data) == 0 && data == 1);
	unit_assert(coro_bus_recv(bus, c2, &data) == 0 && data == 2);
	unit_assert(coro_bus_recv(bus, c2, &data) == 0 && data == 4);
	unit_assert(broadcast_join(&ctx) == 0);

	unit_msg("cleanup");
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 999);
	unit_assert(coro_bus_recv(bus, c2, &data) == 0 && data == 999);
	unit_assert(coro_bus_recv(bus, c3, &data) == 0 && data == 999);

	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	unit_assert(coro_bus_try_recv(bus, c2, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	unit_assert(coro_bus_try_recv(bus, c3, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);

	coro_bus_channel_close(bus, c1);
	coro_bus_channel_close(bus, c2);
	coro_bus_channel_close(bus, c3);

	coro_bus_delete(bus);
	unit_test_finish();
#endif
}

static void
test_broadcast_blocking_drop_channel_during_wait(void)
{
#if NEED_BROADCAST
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();

	unit_msg("create some channels full with data");
	int c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	int c2 = coro_bus_channel_open(bus, 2);
	unit_assert(c2 >= 0);

	unit_assert(coro_bus_send(bus, c1, 0) == 0);
	unit_assert(coro_bus_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_send(bus, c2, 2) == 0);
	unit_assert(coro_bus_send(bus, c2, 3) == 0);

	unit_msg("start a broadcast");
	struct ctx_broadcast ctx;
	broadcast_start(&ctx, bus, 999);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);

	unit_msg("drop one channel");
	coro_bus_channel_close(bus, c1);

	unit_msg("unblock the other one");
	unsigned data = 0;
	unit_assert(coro_bus_recv(bus, c2, &data) == 0 && data == 2);
	unit_assert(coro_bus_recv(bus, c2, &data) == 0 && data == 3);

	unit_msg("finish the broadcast");
	unit_assert(broadcast_join(&ctx) == 0);

	unit_msg("data was sent to the remaining channel");
	unit_assert(coro_bus_recv(bus, c2, &data) == 0 && data == 999);
	unit_assert(coro_bus_try_recv(bus, c2, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);

	coro_bus_channel_close(bus, c2);
	coro_bus_delete(bus);
	unit_test_finish();
#endif
}

////////////////////////////////////////////////////////////////////////////////

static void
test_send_vector_basic(void)
{
#if NEED_BATCH
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();

	unit_msg("channel never existed");
	unsigned data3[3] = {1, 2, 3};
	unit_assert(coro_bus_send_v(bus, 0, data3, 3) < 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(coro_bus_try_send_v(bus, 0, data3, 3) < 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);

	unit_msg("channel did exist");
	coro_bus_channel_close(bus, coro_bus_channel_open(bus, 3));
	unit_assert(coro_bus_send_v(bus, 0, data3, 3) < 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(coro_bus_try_send_v(bus, 0, data3, 3) < 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);

	unit_msg("channel is full");
	int c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	unit_assert(coro_bus_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_send(bus, c1, 2) == 0);
	unit_assert(coro_bus_try_send_v(bus, c1, data3, 3) < 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	unsigned data = 123;
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 1);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 2);
	coro_bus_channel_close(bus, c1);

	unit_msg("channel is partially full");
	c1 = coro_bus_channel_open(bus, 5);
	unit_assert(c1 >= 0);
	unit_assert(coro_bus_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_send(bus, c1, 2) == 0);
	unit_assert(coro_bus_send(bus, c1, 3) == 0);
	data3[0] = 4, data3[1] = 5, data3[2] = 6;
	unit_assert(coro_bus_send_v(bus, c1, data3, 3) == 2);
	for (unsigned i = 1; i <= 5; ++i)
		unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == i);
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	unit_msg("same with try-send-v");
	c1 = coro_bus_channel_open(bus, 5);
	unit_assert(c1 >= 0);
	unit_assert(coro_bus_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_send(bus, c1, 2) == 0);
	unit_assert(coro_bus_send(bus, c1, 3) == 0);
	data3[0] = 4, data3[1] = 5, data3[2] = 6;
	unit_assert(coro_bus_try_send_v(bus, c1, data3, 3) == 2);
	for (unsigned i = 1; i <= 5; ++i)
		unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == i);
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	unit_msg("normal full send");
	c1 = coro_bus_channel_open(bus, 10);
	unit_assert(c1 >= 0);
	data3[0] = 1, data3[1] = 2, data3[2] = 3;
	unit_assert(coro_bus_send_v(bus, c1, data3, 3) == 3);
	unsigned data4[4] = {4, 5, 6, 7};
	unit_assert(coro_bus_try_send_v(bus, c1, data4, 4) == 4);
	for (unsigned i = 1; i <= 7; ++i)
		unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == i);
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	unit_msg("partial send-v wakes up a waiting receiver");
	c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	struct ctx_recv ctx;
	recv_start(&ctx, bus, c1, &data);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);
	unit_assert(coro_bus_send_v(bus, c1, data3, 3) == 2);
	unit_assert(recv_join(&ctx) == 0 && data == 1);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 2);
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	unit_msg("same with try-send-v");
	c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	recv_start(&ctx, bus, c1, &data);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);
	unit_assert(coro_bus_try_send_v(bus, c1, data3, 3) == 2);
	unit_assert(recv_join(&ctx) == 0 && data == 1);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 2);
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	coro_bus_delete(bus);
	unit_test_finish();
#endif
}

////////////////////////////////////////////////////////////////////////////////

#if NEED_BATCH
struct ctx_send_v {
	struct coro_bus *bus;
	int channel;
	const unsigned *data;
	unsigned count;
	int rc;
	enum coro_bus_error_code err;
	bool is_started;
	bool is_done;
	struct coro *worker;
};

static void *
send_v_f(void *arg)
{
	struct ctx_send_v *ctx = (decltype(ctx))arg;
	ctx->is_started = true;
	ctx->rc = coro_bus_send_v(ctx->bus, ctx->channel, ctx->data,
		ctx->count);
	ctx->err = coro_bus_errno();
	ctx->is_done = true;
	return NULL;
}

static void
send_v_start(struct ctx_send_v *ctx, struct coro_bus *bus, int channel,
	const unsigned *data, unsigned count)
{
	ctx->bus = bus;
	ctx->channel = channel;
	ctx->data = data;
	ctx->count = count;
	ctx->rc = -1;
	ctx->err = CORO_BUS_ERR_NONE;
	ctx->is_started = false;
	ctx->is_done = false;
	ctx->worker = coro_new(send_v_f, ctx);
}

static int
send_v_join(struct ctx_send_v *ctx)
{
	unit_assert(coro_join(ctx->worker) == NULL);
	unit_assert(ctx->is_done);
	coro_bus_errno_set(ctx->err);
	return ctx->rc;
}
#endif

static void
test_send_vector_blocking(void)
{
#if NEED_BATCH
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();
	int c1 = coro_bus_channel_open(bus, 3);
	unit_assert(c1 >= 0);

	unit_msg("fill the channel");
	unsigned data3[3] = {1, 2, 3};
	unit_assert(coro_bus_send_v(bus, c1, data3, 3) == 3);

	unit_msg("start a blocking send-v");
	unsigned data4[4] = {4, 5, 6, 7};
	struct ctx_send_v ctx;
	send_v_start(&ctx, bus, c1, data4, 4);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);

	unit_msg("spurious wakeup");
	coro_wakeup(ctx.worker);
	coro_yield();
	unit_assert(!ctx.is_done);

	unit_msg("free some space");
	unsigned data = 0;
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 1);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 2);
	unit_assert(send_v_join(&ctx) == 2);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 3);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 4);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 5);

	coro_bus_channel_close(bus, c1);
	coro_bus_delete(bus);
	unit_test_finish();
#endif
}

////////////////////////////////////////////////////////////////////////////////

#if NEED_BATCH
static void *
send_v_all_f(void *arg)
{
	struct ctx_send_v *ctx = (decltype(ctx))arg;
	ctx->is_started = true;
	while (ctx->count > 0) {
		int rc = coro_bus_send_v(ctx->bus, ctx->channel, ctx->data,
			ctx->count);
		if (rc < 0) {
			ctx->err = coro_bus_errno();
			ctx->rc = -1;
			break;
		}
		ctx->rc += rc;
		ctx->data += rc;
		ctx->count -= rc;
	}
	ctx->is_done = true;
	return NULL;
}

static void
send_v_all_start(struct ctx_send_v *ctx, struct coro_bus *bus, int channel,
	const unsigned *data, unsigned count)
{
	ctx->bus = bus;
	ctx->channel = channel;
	ctx->data = data;
	ctx->count = count;
	ctx->rc = 0;
	ctx->err = CORO_BUS_ERR_NONE;
	ctx->is_started = false;
	ctx->is_done = false;
	ctx->worker = coro_new(send_v_all_f, ctx);
}
#endif

static void
test_send_vector_blocking_recv_many(void)
{
#if NEED_BATCH
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();
	const unsigned limit = 3;
	int c1 = coro_bus_channel_open(bus, limit);
	unit_assert(c1 >= 0);

	const unsigned coro_count = 10;
	struct ctx_send_v ctx[coro_count];

	const unsigned data_per_coro = 100;
	const unsigned data_count = coro_count * data_per_coro;
	unsigned *data = new unsigned[data_count];
	for (unsigned i = 0; i < data_count; ++i)
		data[i] = i;
	unit_msg("start many coros");
	for (unsigned i = 0; i < coro_count; ++i) {
		unsigned *data_begin = data + i * data_per_coro;
		send_v_all_start(&ctx[i], bus, c1, data_begin, data_per_coro);
	}

	unit_msg("ensure they are all running but not finished yet");
	coro_yield();
	for (unsigned i = 0; i < coro_count; ++i) {
		unit_assert(ctx[i].is_started);
		unit_assert(!ctx[i].is_done);
	}

	unit_msg("receive all the messages");
	bool *results = new bool[data_count];
	memset(results, 0, data_count * sizeof(*results));
	for (unsigned i = 0; i < data_count; ++i) {
		unsigned data = 0;
		unit_assert(coro_bus_recv(bus, c1, &data) == 0);
		unit_assert(data < data_count);
		unit_assert(!results[data]);
		results[data] = true;
	}

	unit_msg("finalize all the senders");
	for (unsigned i = 0; i < coro_count; ++i) {
		int rc = send_v_join(&ctx[i]);
		unit_assert(rc > 0);
		unit_assert((unsigned)rc == data_per_coro);
	}

	unit_msg("check that nothing is lost");
	for (unsigned i = 0; i < data_count; ++i)
		unit_assert(results[i]);

	delete[] results;
	delete[] data;
	coro_bus_channel_close(bus, c1);
	coro_bus_delete(bus);
	unit_test_finish();
#endif
}

////////////////////////////////////////////////////////////////////////////////

static void
test_recv_vector_basic(void)
{
#if NEED_BATCH
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();

	unit_msg("channel never existed");
	unsigned data3[3] = {0};
	unit_assert(coro_bus_recv_v(bus, 0, data3, 3) < 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(coro_bus_try_recv_v(bus, 0, data3, 3) < 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);

	unit_msg("channel did exist");
	coro_bus_channel_close(bus, coro_bus_channel_open(bus, 3));
	unit_assert(coro_bus_recv_v(bus, 0, data3, 3) < 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);
	unit_assert(coro_bus_try_recv_v(bus, 0, data3, 3) < 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_NO_CHANNEL);

	unit_msg("channel is empty");
	int c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	unit_assert(coro_bus_try_recv_v(bus, c1, data3, 3) < 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	unit_msg("channel is smaller than the recv capacity");
	c1 = coro_bus_channel_open(bus, 5);
	unit_assert(c1 >= 0);
	unit_assert(coro_bus_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_send(bus, c1, 2) == 0);
	unit_assert(coro_bus_recv_v(bus, c1, data3, 3) == 2);
	unit_assert(data3[0] == 1 && data3[1] == 2);
	unit_assert(coro_bus_send(bus, c1, 3) == 0);
	unit_assert(coro_bus_send(bus, c1, 4) == 0);
	unit_assert(coro_bus_try_recv_v(bus, c1, data3, 3) == 2);
	unit_assert(data3[0] == 3 && data3[1] == 4);
	unsigned data = 0;
	unit_assert(coro_bus_try_recv(bus, c1, &data) < 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	unit_msg("normal full recv");
	c1 = coro_bus_channel_open(bus, 10);
	unit_assert(c1 >= 0);
	for (unsigned i = 1; i <= 10; ++i)
		unit_assert(coro_bus_send(bus, c1, i) == 0);
	unit_assert(coro_bus_recv_v(bus, c1, data3, 3) == 3);
	unit_assert(data3[0] == 1 && data3[1] == 2 && data3[2] == 3);
	unit_assert(coro_bus_try_recv_v(bus, c1, data3, 3) == 3);
	unit_assert(data3[0] == 4 && data3[1] == 5 && data3[2] == 6);
	for (unsigned i = 7; i <= 10; ++i)
		unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == i);
	unit_assert(coro_bus_try_recv_v(bus, c1, data3, 3) < 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	unit_msg("partial recv-v wakes up a waiting sender");
	c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	unit_assert(coro_bus_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_send(bus, c1, 2) == 0);
	struct ctx_send ctx;
	send_start(&ctx, bus, c1, 3);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);
	unsigned data4[4] = {0};
	unit_assert(coro_bus_recv_v(bus, c1, data4, 4) == 2);
	unit_assert(data4[0] == 1 && data4[1] == 2);
	unit_assert(send_join(&ctx) == 0);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 3);
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	unit_msg("same with try-recv-v");
	c1 = coro_bus_channel_open(bus, 2);
	unit_assert(c1 >= 0);
	unit_assert(coro_bus_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_send(bus, c1, 2) == 0);
	send_start(&ctx, bus, c1, 3);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);
	memset(data4, 0, sizeof(data4));
	unit_assert(coro_bus_try_recv_v(bus, c1, data4, 4) == 2);
	unit_assert(data4[0] == 1 && data4[1] == 2);
	unit_assert(send_join(&ctx) == 0);
	unit_assert(coro_bus_recv(bus, c1, &data) == 0 && data == 3);
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);
	coro_bus_channel_close(bus, c1);

	coro_bus_delete(bus);
	unit_test_finish();
#endif
}

////////////////////////////////////////////////////////////////////////////////

#if NEED_BATCH
struct ctx_recv_v {
	struct coro_bus *bus;
	int channel;
	unsigned *data;
	unsigned count;
	int rc;
	enum coro_bus_error_code err;
	bool is_started;
	bool is_done;
	struct coro *worker;
};

static void *
recv_v_f(void *arg)
{
	struct ctx_recv_v *ctx = (decltype(ctx))arg;
	ctx->is_started = true;
	ctx->rc = coro_bus_recv_v(ctx->bus, ctx->channel, ctx->data,
		ctx->count);
	ctx->err = coro_bus_errno();
	ctx->is_done = true;
	return NULL;
}

static void
recv_v_start(struct ctx_recv_v *ctx, struct coro_bus *bus, int channel,
	unsigned *data, unsigned count)
{
	ctx->bus = bus;
	ctx->channel = channel;
	ctx->data = data;
	ctx->count = count;
	ctx->rc = -1;
	ctx->err = CORO_BUS_ERR_NONE;
	ctx->is_started = false;
	ctx->is_done = false;
	ctx->worker = coro_new(recv_v_f, ctx);
}

static int
recv_v_join(struct ctx_recv_v *ctx)
{
	unit_assert(coro_join(ctx->worker) == NULL);
	unit_assert(ctx->is_done);
	coro_bus_errno_set(ctx->err);
	return ctx->rc;
}
#endif

static void
test_recv_vector_blocking(void)
{
#if NEED_BATCH
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();
	int c1 = coro_bus_channel_open(bus, 10);
	unit_assert(c1 >= 0);

	unit_msg("start a blocking recv-v");
	unsigned data4[4] = {0};
	struct ctx_recv_v ctx;
	recv_v_start(&ctx, bus, c1, data4, 4);
	coro_yield();
	unit_assert(ctx.is_started && !ctx.is_done);

	unit_msg("spurious wakeup");
	coro_wakeup(ctx.worker);
	coro_yield();
	unit_assert(!ctx.is_done);

	unit_msg("push some data");
	unit_assert(coro_bus_send(bus, c1, 1) == 0);
	unit_assert(coro_bus_send(bus, c1, 2) == 0);
	unit_assert(coro_bus_send(bus, c1, 3) == 0);

	unit_msg("finish the recv");
	unit_assert(recv_v_join(&ctx) == 3);
	unit_assert(data4[0] == 1 && data4[1] == 2 && data4[2] == 3);
	unsigned data = 0;
	unit_assert(coro_bus_try_recv(bus, c1, &data) != 0);
	unit_assert(coro_bus_errno() == CORO_BUS_ERR_WOULD_BLOCK);

	coro_bus_channel_close(bus, c1);
	coro_bus_delete(bus);
	unit_test_finish();
#endif
}

////////////////////////////////////////////////////////////////////////////////

#if NEED_BATCH
static void *
recv_v_all_f(void *arg)
{
	struct ctx_recv_v *ctx = (decltype(ctx))arg;
	ctx->is_started = true;
	while (ctx->count > 0) {
		int rc = coro_bus_recv_v(ctx->bus, ctx->channel, ctx->data,
			ctx->count);
		if (rc < 0) {
			ctx->err = coro_bus_errno();
			ctx->rc = -1;
			break;
		}
		ctx->rc += rc;
		ctx->data += rc;
		ctx->count -= rc;
	}
	ctx->is_done = true;
	return NULL;
}

static void
recv_v_all_start(struct ctx_recv_v *ctx, struct coro_bus *bus, int channel,
	unsigned *data, unsigned count)
{
	ctx->bus = bus;
	ctx->channel = channel;
	ctx->data = data;
	ctx->count = count;
	ctx->rc = 0;
	ctx->err = CORO_BUS_ERR_NONE;
	ctx->is_started = false;
	ctx->is_done = false;
	ctx->worker = coro_new(recv_v_all_f, ctx);
}
#endif

static void
test_recv_vector_blocking_recv_many(void)
{
#if NEED_BATCH
	unit_test_start();
	struct coro_bus *bus = coro_bus_new();
	const unsigned limit = 3;
	int c1 = coro_bus_channel_open(bus, limit);
	unit_assert(c1 >= 0);

	const unsigned coro_count = 10;
	struct ctx_recv_v ctx[coro_count];
	const unsigned data_per_coro = 100;
	const unsigned data_count = coro_count * data_per_coro;
	unsigned *data = new unsigned[data_count];
	memset(data, 0, sizeof(*data) * data_count);

	unit_msg("start many coros");
	for (unsigned i = 0; i < coro_count; ++i) {
		unsigned *data_begin = data + i * data_per_coro;
		recv_v_all_start(&ctx[i], bus, c1, data_begin, data_per_coro);
	}

	unit_msg("ensure they are all running but not finished yet");
	coro_yield();
	for (unsigned i = 0; i < coro_count; ++i) {
		unit_assert(ctx[i].is_started);
		unit_assert(!ctx[i].is_done);
	}

	unit_msg("send all the messages");
	for (unsigned i = 0; i < data_count; ++i)
		unit_assert(coro_bus_send(bus, c1, i) == 0);

	unit_msg("finalize all the senders");
	for (unsigned i = 0; i < coro_count; ++i) {
		int rc = recv_v_join(&ctx[i]);
		unit_assert(rc > 0);
		unit_assert((unsigned)rc == data_per_coro);
	}

	unit_msg("check that nothing is lost");
	bool *results = new bool[data_count];
	memset(results, 0, sizeof(*results) * data_count);
	for (unsigned i = 0; i < data_count; ++i) {
		unit_assert(!results[data[i]]);
		results[data[i]] = true;
	}
	delete[] results;

	delete[] data;
	coro_bus_channel_close(bus, c1);
	coro_bus_delete(bus);
	unit_test_finish();
#endif
}

////////////////////////////////////////////////////////////////////////////////

static void *
coro_main_f(void *arg)
{
	(void)arg;
	test_basic();
	test_channel_reopen();
	test_multiple_channels();

	test_send_basic();
	test_send_blocking();
	test_send_blocking_recv_many();

	test_recv_basic();
	test_recv_blocking();
	test_recv_blocking_send_many();

	test_stress_send_recv_concurrent();
	test_send_recv_very_many();
	test_wakeup_on_close();
	test_close_non_empty_bus();

	test_broadcast_basic();
	test_broadcast_blocking_basic();
	test_broadcast_blocking_drop_channel_during_wait();

	test_send_vector_basic();
	test_send_vector_blocking();
	test_send_vector_blocking_recv_many();

	test_recv_vector_basic();
	test_recv_vector_blocking();
	test_recv_vector_blocking_recv_many();
	return NULL;
}

int
main(int argc, char **argv)
{
	if (doCmdMaxPoints(argc, argv)) {
		int result = 15;
#if NEED_BROADCAST
		result += 5;
#endif
#if NEED_BATCH
		result += 5;
#endif
		printf("%d\n", result);
		return 0;
	}
	coro_sched_init();
	struct coro *main_coro = coro_new(coro_main_f, NULL);
	coro_sched_run();
	void *rc = coro_join(main_coro);
	unit_check(rc == NULL, "main coro rc");
	coro_sched_destroy();
	return 0;
}
