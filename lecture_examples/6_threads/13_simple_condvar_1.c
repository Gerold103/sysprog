#include <assert.h>
#include <linux/futex.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

static void
futex_wake(int *val)
{
	syscall(SYS_futex, val, FUTEX_WAKE, 1, NULL, NULL, 0);
}

static void
futex_wait(int *val, int old)
{
	syscall(SYS_futex, val, FUTEX_WAIT, old, NULL, NULL, 0);
}

struct condvar {
	int sequence;
};

static void
condvar_signal(struct condvar *cond)
{
	__atomic_add_fetch(&cond->sequence, 1, __ATOMIC_RELEASE);
	futex_wake(&cond->sequence);
}

static void
condvar_wait(struct condvar *cond)
{
	// This will deadlock, because will miss 'signal()' sometimes.
	// Uncomment the next line to increase the likelyhood.
	//
	// usleep(100);
	//
	int old = __atomic_load_n(&cond->sequence, __ATOMIC_ACQUIRE);
	futex_wait(&cond->sequence, old);
}

static void
condvar_create(struct condvar *cond)
{
	cond->sequence = 0;
}

struct state {
	pthread_mutex_t mutex;
	struct condvar send_cond;
	struct condvar recv_cond;

	int counter_send;
	int counter_recv;
	int target;
};

static int thread_send_id = 0;
static int thread_recv_id = 0;

static void *
thread_send_f(void *arg)
{
	struct state *state = arg;
	int tid = __atomic_add_fetch(&thread_send_id, 1, __ATOMIC_RELAXED);
	printf("send %d: start\n", tid);

	pthread_mutex_lock(&state->mutex);
	while (state->counter_send < state->target) {
		while (state->counter_send > state->counter_recv) {
			pthread_mutex_unlock(&state->mutex);
			condvar_wait(&state->send_cond);
			pthread_mutex_lock(&state->mutex);
		}
		++state->counter_send;
		printf("send %d: sent %d\n", tid, state->counter_send);
		condvar_signal(&state->recv_cond);
	}
	pthread_mutex_unlock(&state->mutex);
}

static void *
thread_recv_f(void *arg)
{
	struct state *state = arg;
	int tid = __atomic_add_fetch(&thread_recv_id, 1, __ATOMIC_RELAXED);
	printf("recv %d: start\n", tid);

	pthread_mutex_lock(&state->mutex);
	while (state->counter_recv < state->target) {
		while (state->counter_send == state->counter_recv) {
			pthread_mutex_unlock(&state->mutex);
			condvar_wait(&state->recv_cond);
			pthread_mutex_lock(&state->mutex);
		}
		assert(state->counter_send == state->counter_recv + 1);
		++state->counter_recv;
		printf("recv %d: received %d\n", tid, state->counter_recv);
		condvar_signal(&state->send_cond);
	}
	pthread_mutex_unlock(&state->mutex);
}

int
main()
{
	struct state state;
	pthread_mutex_init(&state.mutex, NULL);
	condvar_create(&state.send_cond);
	condvar_create(&state.recv_cond);
	state.counter_send = 0;
	state.counter_recv = 0;
	state.target = 1000000;

	const int thread_count = 5;
	pthread_t tid_send[thread_count];
	for (int i = 0; i < thread_count; ++i)
		pthread_create(&tid_send[i], NULL, thread_send_f, &state);

	pthread_t tid_recv[thread_count];
	for (int i = 0; i < thread_count; ++i)
		pthread_create(&tid_recv[i], NULL, thread_recv_f, &state);

	for (int i = 0; i < thread_count; ++i) {
		pthread_join(tid_send[i], NULL);
		pthread_join(tid_recv[i], NULL);
	}
	return 0;
}
