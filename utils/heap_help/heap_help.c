#define _GNU_SOURCE
#include "heap_help.h"

#include <assert.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *(*default_malloc)(size_t) = NULL;
static void (*default_free)() = NULL;
static void *(*default_calloc)(size_t, size_t) = NULL;
static void *(*default_realloc)(void *, size_t) = NULL;
static char *(*default_strdup)(const char *) = NULL;
static ssize_t (*default_getline)(char **, size_t *, FILE *) = NULL;

static int64_t alloc_count = 0;
static bool global_lock = false;

static inline void
alloc_count_inc()
{
	__atomic_add_fetch(&alloc_count, 1, __ATOMIC_SEQ_CST);
}

static inline void
alloc_count_sub()
{
	int64_t old = __atomic_sub_fetch(&alloc_count, 1, __ATOMIC_SEQ_CST);
	if (old >= 0)
		return;
	printf("Double-free detected\n");
	exit(1);
}

static void
heaph_lock(void)
{
	// It has to be a spinlock. So as not to depend on pthread.
	while (true) {
		int i = 0;
		while (i++ < 1000) {
			if (!__atomic_test_and_set(&global_lock,
						   __ATOMIC_SEQ_CST))
				return;
		}
		usleep(10);
	}
}

static void
heaph_unlock(void)
{
	assert(global_lock);
	__atomic_clear(&global_lock, __ATOMIC_SEQ_CST);
}

static void
heaph_atexit(void)
{
	// On Mac somehow this atexit handler gets executed 2 times. Have to
	// manually filter out all calls except for the first one.
	static bool is_done = false;
	if (__atomic_test_and_set(&is_done, __ATOMIC_SEQ_CST))
		return;
	int64_t count = __atomic_load_n(&alloc_count, __ATOMIC_SEQ_CST);
	if (count != 0)
		printf("Found %lld leaks\n", (long long)count);
}

static void
heaph_init(void)
{
	default_getline = dlsym(RTLD_NEXT, "getline");
	default_strdup = dlsym(RTLD_NEXT, "strdup");
	default_malloc = dlsym(RTLD_NEXT, "malloc");
	default_calloc = dlsym(RTLD_NEXT, "calloc");
	default_realloc = dlsym(RTLD_NEXT, "realloc");
	default_free = dlsym(RTLD_NEXT, "free");

	const char *hh_report = getenv("HHREPORT");
	if (hh_report != NULL && strcmp(hh_report, "1") == 0)
		atexit(heaph_atexit);
}

static void
heaph_touch(void)
{
	static bool is_done = false;
	if (__atomic_load_n(&is_done, __ATOMIC_SEQ_CST))
		return;

	heaph_lock();
	heaph_init();
	bool old = __atomic_exchange_n(&is_done, true, __ATOMIC_SEQ_CST);
	heaph_unlock();
	assert(!old);
}

ssize_t
getline(char **linep, size_t *linecapp, FILE *stream)
{
	heaph_touch();
	char *line_old = *linep;
	ssize_t res = default_getline(linep, linecapp, stream);
	if (line_old == NULL && *linep != NULL)
		alloc_count_inc();
	return res;
}

char *
strdup(const char *ptr)
{
	heaph_touch();
	char *res = default_strdup(ptr);
	if (res != NULL)
		alloc_count_inc();
	return res;
}

void *
malloc(size_t size)
{
	heaph_touch();
	void *res = default_malloc(size);
	if (res != NULL)
		alloc_count_inc();
	return res;
}

void *
calloc(size_t num, size_t size)
{
	heaph_touch();
	void *res = default_calloc(num, size);
	if (res != NULL)
		alloc_count_inc();
	return res;
}

void *
realloc(void *ptr, size_t size)
{
	heaph_touch();
	void *res = default_realloc(ptr, size);
	if (ptr == NULL && res != NULL)
		alloc_count_inc();
	return res;
}

void
free(void *ptr)
{
	if (ptr == NULL)
		return;
	heaph_touch();
	alloc_count_sub();
	default_free(ptr);
}

uint64_t
heaph_get_alloc_count(void)
{
	return (uint64_t)__atomic_load_n(&alloc_count, __ATOMIC_SEQ_CST);
}
