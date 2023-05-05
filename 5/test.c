#include "unit.h"
#include "chat.h"
#include "chat_client.h"
#include "chat_server.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>

static uint16_t
server_get_port(const struct chat_server *s)
{
	int sock = chat_server_get_socket(s);
	unit_fail_if(sock < 0);
	struct sockaddr_storage addr;
	socklen_t len = sizeof(addr);
	int rc = getsockname(sock, (void *)&addr, &len);
	unit_fail_if(rc != 0);
	if (addr.ss_family == AF_INET)
		return ntohs(((struct sockaddr_in *)&addr)->sin_port);
	unit_fail_if(addr.ss_family != AF_INET6);
	return ntohs(((struct sockaddr_in6 *)&addr)->sin6_port);
}

static inline const char *
make_addr_str(uint16_t port)
{
	static __thread char host[128];
	sprintf(host, "localhost:%u", port);
	return host;
}

static void
server_consume_events(struct chat_server *s)
{
	int rc;
	while ((rc = chat_server_update(s, 0)) == 0)
		{};
	unit_fail_if(rc != CHAT_ERR_TIMEOUT);
}

static void
client_consume_events(struct chat_client *c)
{
	int rc;
	while ((rc = chat_client_update(c, 0)) == 0)
		{};
	unit_fail_if(rc != CHAT_ERR_TIMEOUT);
}

static bool
author_is_eq(const struct chat_message *msg, const char *name)
{
#if NEED_AUTHOR
	return strcmp(msg->author, name) == 0;
#else
	(void)msg;
	(void)name;
	return true;
#endif
}

static void
test_basic(void)
{
	unit_test_start();
	//
	// Delete the server without listen.
	//
	struct chat_server *s = chat_server_new();
	unit_check(chat_server_get_socket(s) < 0, "server no socket");
	unit_check(chat_server_get_events(s) == 0, "server no events");
	unit_check(chat_server_update(s, 0) == CHAT_ERR_NOT_STARTED,
		   "server no update");
	chat_server_delete(s);
	//
	// Delete the server right after listen.
	//
	s = chat_server_new();
	int rc = chat_server_listen(s, 0);
	unit_check(rc == 0, "listen");
	unit_check(chat_server_get_socket(s) >= 0, "server has socket");
	unit_check(server_get_port(s) > 0, "has port");
	unit_check(chat_server_get_events(s) != 0, "has events");
	chat_server_delete(s);
	//
	// Delete the client right away.
	//
	struct chat_client *c1 = chat_client_new("c1");
	unit_check(chat_client_update(c1, 0) == CHAT_ERR_NOT_STARTED,
		   "client no update");
	unit_check(chat_client_get_descriptor(c1) < 0, "client no socket");
	unit_check(chat_client_get_events(c1) == 0, "client no events");
	chat_client_delete(c1);
	//
	// Delete the client after connect.
	//
	s = chat_server_new();
	rc = chat_server_listen(s, 0);
	uint16_t port = server_get_port(s);
	c1 = chat_client_new("c1");
	unit_check(chat_server_get_events(s) == CHAT_EVENT_INPUT,
		   "server needs input");
	unit_check(chat_server_update(s, 0) == CHAT_ERR_TIMEOUT,
		   "server no clients yet");
	unit_check(chat_client_connect(c1, make_addr_str(port)) == 0,
		   "connect");
	unit_check(chat_server_update(s, 0) == 0, "server got client");
	unit_check(chat_server_get_events(s) == CHAT_EVENT_INPUT,
		   "server always needs more input");
	chat_client_delete(c1);
	unit_check(chat_server_update(s, 0) == 0, "server lost client");
	unit_check(chat_server_get_events(s) == CHAT_EVENT_INPUT,
		   "server needs more input");
	//
	// Connect a client and send a message.
	//
	c1 = chat_client_new("c1");
	unit_fail_if(chat_client_connect(c1, make_addr_str(port)) != 0);
	unit_check(chat_client_feed(c1, "hello\n", 6) == 0, "feed to client");
	client_consume_events(c1);
	server_consume_events(s);
	struct chat_message *msg = chat_server_pop_next(s);
	unit_check(msg != NULL, "server got msg");
	unit_check(strcmp(msg->data, "hello") == 0, "msg data");
	unit_check(author_is_eq(msg, "c1"), "msg author");
	chat_message_delete(msg);
	//
	// Send a non-zero terminated message.
	//
	// Yes, 5, not 10. Send only "msg1\n".
	unit_check(chat_client_feed(c1, "msg1\nmsg2\n", 5) == 0,
		   "feed a part of str");
	client_consume_events(c1);
	server_consume_events(s);
	msg = chat_server_pop_next(s);
	unit_check(strcmp(msg->data, "msg1") == 0, "msg data");
	unit_check(author_is_eq(msg, "c1"), "msg author");
	chat_message_delete(msg);
	unit_check(chat_server_pop_next(s) == NULL, "no more messages");
	chat_client_delete(c1);
	chat_server_delete(s);

	unit_test_finish();
}

static void
test_big_messages(void)
{
	unit_test_start();

	struct chat_server *s = chat_server_new();
	unit_fail_if(chat_server_listen(s, 0) != 0);
	uint16_t port = server_get_port(s);
	struct chat_client *c1 = chat_client_new("c1");
	unit_fail_if(chat_client_connect(c1, make_addr_str(port)) != 0);

	uint32_t len = 1024 * 1024;
	uint32_t size = len + 1;
	char *data = malloc(size);
	for (uint32_t i = 0; i < len; ++i)
		data[i] = 'a' + i % ('z' - 'a' + 1);
	data[len] = '\n';
	int rc;
	struct chat_message *msg;
	int count = 100;
	int real_count = 0;
	for (int i = 0; i < count; ++i) {
		unit_fail_if(chat_client_feed(c1, data, size) != 0);
		rc = chat_client_update(c1, 0);
		unit_fail_if(rc != 0 && rc != CHAT_ERR_TIMEOUT);
		rc = chat_server_update(s, 0);
		unit_fail_if(rc != 0 && rc != CHAT_ERR_TIMEOUT);
		while ((msg = chat_server_pop_next(s)) != NULL) {
			++real_count;
			size_t msg_len = strlen(msg->data);
			unit_fail_if(len != msg_len);
			unit_fail_if(memcmp(msg->data, data, len) != 0);
			unit_fail_if(!author_is_eq(msg, "c1"));
			chat_message_delete(msg);
		}
	}
	while (true) {
		int rc1 = chat_client_update(c1, 0);
		unit_fail_if(rc1 != 0 && rc1 != CHAT_ERR_TIMEOUT);
		int rc2 = chat_server_update(s, 0);
		unit_fail_if(rc2 != 0 && rc2 != CHAT_ERR_TIMEOUT);
		while ((msg = chat_server_pop_next(s)) != NULL) {
			++real_count;
			size_t msg_len = strlen(msg->data);
			unit_fail_if(len != msg_len);
			unit_fail_if(memcmp(msg->data, data, len) != 0);
			unit_fail_if(!author_is_eq(msg, "c1"));
			chat_message_delete(msg);
		}
		if (rc1 == CHAT_ERR_TIMEOUT && rc2 == CHAT_ERR_TIMEOUT)
			break;
	}
	free(data);
	unit_check(real_count == count, "server got all msgs");
	unit_check(chat_server_pop_next(s) == NULL, "server has no msgs");
	unit_check(chat_client_pop_next(c1) == NULL, "client has no msgs");
	chat_client_delete(c1);
	chat_server_delete(s);

	unit_test_finish();
}

static void
test_multi_feed(void)
{
	unit_test_start();

	struct chat_server *s = chat_server_new();
	unit_fail_if(chat_server_listen(s, 0) != 0);
	uint16_t port = server_get_port(s);
	struct chat_client *c1 = chat_client_new("c1");
	unit_fail_if(chat_client_connect(c1, make_addr_str(port)) != 0);
	// Multiple messages in one string still should be split by '\n'. If
	// there is no '\n' in the end, then data must be accumulated.
	const char *data = "msg1\nmsg2\nmsg";
	unit_check(chat_client_feed(c1, data, strlen(data)) == 0,
		   "feed an incomplete batch");
	client_consume_events(c1);
	server_consume_events(s);
	struct chat_message *msg = chat_server_pop_next(s);
	unit_check(strcmp(msg->data, "msg1") == 0, "msg1");
	unit_check(author_is_eq(msg, "c1"), "msg1 author");
	chat_message_delete(msg);
	msg = chat_server_pop_next(s);
	unit_check(strcmp(msg->data, "msg2") == 0, "msg2");
	unit_check(author_is_eq(msg, "c1"), "msg2 author");
	chat_message_delete(msg);
	unit_check(chat_server_pop_next(s) == NULL, "waiting for msg3");
	unit_check(chat_client_feed(c1, "345", 3) == 0, "feed next part");
	client_consume_events(c1);
	server_consume_events(s);
	unit_check(chat_server_pop_next(s) == NULL, "still waiting for msg3");
	unit_check(chat_client_feed(c1, "678\n", 4) == 0, "feed next part");
	client_consume_events(c1);
	server_consume_events(s);
	msg = chat_server_pop_next(s);
	unit_check(strcmp(msg->data, "msg345678") == 0, "msg3");
	unit_check(author_is_eq(msg, "c1"), "msg3 author");
	chat_message_delete(msg);
	chat_client_delete(c1);
	chat_server_delete(s);

	unit_test_finish();
}

static void
test_multi_client(void)
{
	unit_test_start();

	struct chat_server *s = chat_server_new();
	unit_fail_if(chat_server_listen(s, 0) != 0);
	uint16_t port = server_get_port(s);
	int client_count = 20;
	int msg_count = 100;
	uint32_t id_len = 64;
	uint32_t len = id_len + 1024;
	uint32_t size = len + 1;
	char *data = malloc(size);
	for (uint32_t i = 0; i < len; ++i)
		data[i] = 'a' + i % ('z' - 'a' + 1);
	data[len] = '\n';
	unit_msg("Connect clients");
	struct chat_client **clis = malloc(client_count * sizeof(clis[0]));
	for (int i = 0; i < client_count; ++i) {
		char name[128];
		sprintf(name, "cli_%d", i);
		clis[i] = chat_client_new(name);
		unit_fail_if(chat_client_connect(
			clis[i], make_addr_str(port)) != 0);
		server_consume_events(s);
	}
	unit_msg("Send messages");
	for (int mi = 0; mi < msg_count; ++mi) {
		for (int ci = 0; ci < client_count; ++ci) {
			memset(data, '0', id_len);
			int rc = sprintf(data, "cli_%d_msg_%d ", ci, mi);
			// Ignore terminating zero.
			data[rc] = '0';
			unit_fail_if(chat_client_feed(
				clis[ci], data, size) != 0);
			chat_client_update(clis[ci], 0);
		}
		chat_server_update(s, 0);
	}
	unit_msg("Consume all events");
	while (true)
	{
		bool have_events = false;
		for (int i = 0; i < client_count; ++i) {
			if (chat_client_update(clis[i], 0) == 0)
				have_events = true;
		}
		if (chat_server_update(s, 0) == 0)
			have_events = true;
		if (!have_events)
			break;
	}
	for (int i = 0; i < client_count; ++i)
		chat_client_delete(clis[i]);
	free(clis);
	unit_msg("Check all is delivered");
	int *msg_counts = calloc(client_count, sizeof(msg_counts[0]));
	memset(data, '0', id_len);
	for (int i = 0, end = msg_count * client_count; i < end; ++i) {
		struct chat_message *msg = chat_server_pop_next(s);
		unit_fail_if(msg == NULL);
		int cli_id = -1;
		int msg_id = -1;
		int rc = sscanf(msg->data, "cli_%d_msg_%d ", &cli_id, &msg_id);
		unit_fail_if(rc != 2);
		unit_fail_if(cli_id > client_count || cli_id < 0);
		unit_fail_if(msg_id > msg_count || msg_id < 0);
		unit_fail_if(msg_counts[cli_id] != msg_id);
		++msg_counts[cli_id];

		uint32_t data_len = strlen(msg->data);
		unit_fail_if(data_len != len);
		memset(msg->data, '0', id_len);
		unit_fail_if(memcmp(msg->data, data, len) != 0);

		char name[128];
		sprintf(name, "cli_%d", cli_id);
		unit_fail_if(!author_is_eq(msg, name));

		chat_message_delete(msg);
	}
	free(msg_counts);
	chat_server_delete(s);
	free(data);

	unit_test_finish();
}

struct test_stress_ctx {
	int msg_count;
	uint32_t msg_len;
	int thread_idx;
	uint16_t port;
	bool is_running;
};

static void *
test_stress_worker_f(void *arg)
{
	struct test_stress_ctx *ctx = arg;
	uint32_t id_len = 64;
	uint32_t len = ctx->msg_len;
	uint32_t size = len + 1;
	int thread_idx = __atomic_fetch_add(
		&ctx->thread_idx, 1, __ATOMIC_RELAXED);
	char *data = malloc(size);
	for (uint32_t i = 0; i < len; ++i)
		data[i] = 'a' + i % ('z' - 'a' + 1);
	data[len] = '\n';
	char name[128];
	sprintf(name, "cli_%d", thread_idx);
	struct chat_client *cli = chat_client_new(name);
	unit_fail_if(chat_client_connect(cli, make_addr_str(ctx->port)) != 0);
	for (int i = 0; i < ctx->msg_count; ++i) {
		memset(data, '0', id_len);
		int rc = sprintf(data, "cli_%d_msg_%d ", thread_idx, i);
		// Ignore terminating zero.
		data[rc] = '0';
		unit_fail_if(chat_client_feed(cli, data, size) != 0);
		chat_client_update(cli, 0);
	}
	while (__atomic_load_n(&ctx->is_running, __ATOMIC_RELAXED)) {
		int rc = chat_client_update(cli, 0.1);
		unit_fail_if(rc != 0 && rc != CHAT_ERR_TIMEOUT);
	}
	chat_client_delete(cli);
	free(data);
	return NULL;
}

static void
test_stress(void)
{
	unit_test_start();

	struct chat_server *s = chat_server_new();
	unit_fail_if(chat_server_listen(s, 0) != 0);

	const int client_count = 10;
	struct test_stress_ctx ctx;
	ctx.msg_count = 100;
	ctx.msg_len = 1024;
	ctx.thread_idx = 0;
	ctx.port = server_get_port(s);
	ctx.is_running = true;

	unit_msg("Start client threads");
	pthread_t threads[client_count];
	for (int i = 0; i < client_count; ++i) {
		int rc = pthread_create(&threads[i], NULL,
			test_stress_worker_f, &ctx);
		unit_fail_if(rc != 0);
	}
	int *msg_counts = calloc(client_count, sizeof(msg_counts[0]));
	char *data = malloc(ctx.msg_len + 1);
	for (uint32_t i = 0; i < ctx.msg_len; ++i)
		data[i] = 'a' + i % ('z' - 'a' + 1);
	data[ctx.msg_len] = '\n';
	uint32_t id_len = 64;
	memset(data, '0', id_len);
	unit_msg("Receive all messages");
	for (int i = 0, end = ctx.msg_count * client_count; i < end; ++i) {
		struct chat_message *msg;
		while ((msg = chat_server_pop_next(s)) == NULL) {
			int rc = chat_server_update(s, 0.1);
			unit_fail_if(rc != 0 && rc != CHAT_ERR_TIMEOUT);
		}
		unit_fail_if(msg == NULL);
		int cli_id = -1;
		int msg_id = -1;
		int rc = sscanf(msg->data, "cli_%d_msg_%d ", &cli_id, &msg_id);
		unit_fail_if(rc != 2);
		unit_fail_if(cli_id > client_count || cli_id < 0);
		unit_fail_if(msg_id > ctx.msg_count || msg_id < 0);
		unit_fail_if(msg_counts[cli_id] != msg_id);
		++msg_counts[cli_id];

		uint32_t data_len = strlen(msg->data);
		unit_fail_if(data_len != ctx.msg_len);
		memset(msg->data, '0', id_len);
		unit_fail_if(memcmp(msg->data, data, ctx.msg_len) != 0);

		char name[128];
		sprintf(name, "cli_%d", cli_id);
		unit_fail_if(!author_is_eq(msg, name));

		chat_message_delete(msg);
	}
	free(msg_counts);
	free(data);
	unit_msg("Clean up the clients");
	__atomic_store_n(&ctx.is_running, false, __ATOMIC_RELAXED);
	for (int i = 0; i < client_count; ++i) {
		void *res;
		int rc = pthread_join(threads[i], &res);
		unit_fail_if(rc != 0);
	}
	chat_server_delete(s);

	unit_test_finish();
}

int
main(void)
{
	unit_test_start();

	test_basic();
	test_big_messages();
	test_multi_feed();
	test_multi_client();
	test_stress();

	unit_test_finish();
	return 0;
}
