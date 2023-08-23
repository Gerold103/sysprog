#define _GNU_SOURCE
#include "heap_help.h"

#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#if __APPLE__
#define PLATFORM_IS_APPLE 1
#define PLATFORM_IS_LINUX 0
#else
#define PLATFORM_IS_APPLE 0
#define PLATFORM_IS_LINUX 1
#endif

enum {
	MAX_BACKTRACE_LEN = 64,
	ALLOCATION_BATCH_SIZE = 1024,
};

enum report_mode {
	// Report always. Even when have no leaks.
	MODE_VERBOSE,
	// Report only if there are leaks.
	MODE_LEAKS,
	// Do not report anything.
	MODE_QUIET,
};

// Single allocation done on the heap by a user.
struct allocation {
	void *trace[MAX_BACKTRACE_LEN];
	int trace_size;
	void *mem;
	size_t size;
	struct allocation *next;
};

// Allocation objects are created in batches via mmap(). They can't be easily
// allocated on the heap as that would cause recursion.
struct allocation_batch {
	struct allocation allocs[ALLOCATION_BATCH_SIZE];
	int used;
};

struct symbol {
	const char *file;
	const char *name;
};

static bool init_lock = false;
static bool is_init_done = false;
static bool is_exit_done = false;
static __thread int init_lock_count = 0;
static __thread int depth = 0;
static enum report_mode report_mode = MODE_LEAKS;

// Before the original heap functions are retrieved, there is a dummy static
// allocator working. It is needed because on some platforms the original
// functions getting via dlsym() itself can do allocations. Which means there
// has to be some allocation technique before the symbols are known.
static int static_size = 0;
static int static_used = 0;
static uint8_t* static_buf = NULL;

static bool allocs_lock = false;
static int64_t alloc_count = 0;
static uint64_t alloc_count_total = 0;
static struct allocation *allocs = NULL;
// Unused allocation objects. For re-use.
static struct allocation *alloc_pool = NULL;
// Freshly created allocation objects. Taken from here when the pool is empty.
static struct allocation_batch *alloc_batch = NULL;

static void *(*default_malloc)(size_t) = NULL;
static void (*default_free)(void *) = NULL;
static void *(*default_calloc)(size_t, size_t) = NULL;
static void *(*default_realloc)(void *, size_t) = NULL;
static char *(*default_strdup)(const char *) = NULL;
static ssize_t (*default_getline)(char **, size_t *, FILE *) = NULL;
static int (*default_getaddrinfo)(
	const char *, const char *, const struct addrinfo *,
	struct addrinfo **) = NULL;
static void (*default_freeaddrinfo)(struct addrinfo *) = NULL;

static void
heaph_printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	char msg[1024];
	vsnprintf(msg, sizeof(msg), format, args);
	va_end(args);
	write(STDOUT_FILENO, msg, strlen(msg));
}

static void
heaph_assert_do(bool flag, const char *expr, int line)
{
	if (flag)
		return;
	// Keep volatile errors on the stack. To see them in debugger.
	volatile int err = errno;
	volatile const char *err_msg = strerror(err);
	(void)err;
	(void)err_msg;
	heaph_printf("\n");
	heaph_printf("HH: assertion failure, line %d\n", line);
	heaph_printf("HH: ");
	write(STDOUT_FILENO, expr, strlen(expr));
	heaph_printf("\n");
	abort();
}

#define heaph_assert(expr) heaph_assert_do((expr), #expr, __LINE__)

// Have to use spinlocks so as not to depend on pthread.
static void
spinlock_acq(bool *lock)
{
	while (true) {
		int i = 0;
		while (i++ < 1000) {
			if (!__atomic_test_and_set(lock, __ATOMIC_SEQ_CST))
				return;
		}
		usleep(10);
	}
}

static void
spinlock_rel(bool *lock)
{
	heaph_assert(*lock);
	__atomic_clear(lock, __ATOMIC_SEQ_CST);
}

static bool
alloc_is_static(const void *ptr)
{
	return ptr != NULL && ptr >= (void *)static_buf &&
		ptr <= (void *)(static_buf + static_size);
}

static void
alloc_trace_new(void *ptr, size_t size)
{
	if (alloc_is_static(ptr))
		return;
	// Have to skip allocations inside allocations. The reason is that on
	// some platforms it was seen that getaddrinfo() internal allocations
	// were not intercepted, but freeaddrinfo() did call free() which was
	// caught. As a result there was a free() for an untracked allocation.
	if (depth > 1)
		return;
	heaph_assert(depth > 0);
	heaph_assert(is_init_done);
	if (is_exit_done)
		return;
	spinlock_acq(&allocs_lock);
	struct allocation *a = alloc_pool;
	if (a != NULL) {
		alloc_pool = a->next;
	} else {
		if (alloc_batch == NULL ||
		    alloc_batch->used == ALLOCATION_BATCH_SIZE) {
			alloc_batch = mmap(NULL,
				sizeof(*alloc_batch), PROT_READ | PROT_WRITE,
				MAP_ANON | MAP_PRIVATE, -1, 0);
			heaph_assert(alloc_batch != MAP_FAILED);
			alloc_batch->used = 0;
		} else {
			heaph_assert(alloc_batch->used < ALLOCATION_BATCH_SIZE);
		}
		a = &alloc_batch->allocs[alloc_batch->used++];
	}
	spinlock_rel(&allocs_lock);

	a->mem = ptr;
	a->size = size;
	if (depth == 1)
		a->trace_size = backtrace(a->trace, MAX_BACKTRACE_LEN);
	else
		a->trace_size = 0;
	heaph_assert(a->trace_size >= 0);

	spinlock_acq(&allocs_lock);
	a->next = allocs;
	allocs = a;
	++alloc_count;
	++alloc_count_total;
	spinlock_rel(&allocs_lock);
}

static void
alloc_free(void *ptr)
{
	if (alloc_is_static(ptr))
		return;
	if (depth > 1)
		return;
	heaph_assert(is_init_done);
	if (is_exit_done)
		return;
	spinlock_acq(&allocs_lock);
	struct allocation *a = allocs;
	struct allocation *prev = NULL;
	while (a != NULL) {
		if (a->mem == ptr) {
			if (prev == NULL)
				allocs = a->next;
			else
				prev->next = a->next;
			a->next = alloc_pool;
			alloc_pool = a;
			int64_t new_count = --alloc_count;
			spinlock_rel(&allocs_lock);

			heaph_assert(new_count >= 0 && "freeing bad memory");
			return;
		}
		prev = a;
		a = a->next;
	}
	spinlock_rel(&allocs_lock);
	heaph_assert(!"freeing bad memory");
}

static void
static_allocator_touch(void)
{
	if (static_buf != NULL)
		return;
	static_size = 16 * 1024;
	static_buf = mmap(NULL, static_size, PROT_READ | PROT_WRITE,
			  MAP_ANON | MAP_PRIVATE, -1, 0);
	heaph_assert(static_buf != MAP_FAILED);
}

static void *
static_malloc(size_t size)
{
	heaph_assert(!is_init_done);
	heaph_assert(init_lock);
	static_allocator_touch();
	heaph_assert(static_size >= static_used);
	heaph_assert(size < (size_t)(static_size - static_used));
	void *res = static_buf + static_used;
	static_used += size;
	return res;
}

static void
static_free(void *ptr)
{
	heaph_assert(!is_init_done);
	heaph_assert(init_lock);
	static_allocator_touch();
	if (ptr != NULL)
		heaph_assert(alloc_is_static(ptr));
	(void)ptr;
}

static void *
static_calloc(size_t count, size_t size)
{
	size *= count;
	void *res = static_malloc(size);
	memset(res, 0, size);
	return res;
}

static void
heaph_lock(void)
{
	if (init_lock_count == 0)
		spinlock_acq(&init_lock);
	++init_lock_count;
}

static void
heaph_unlock(void)
{
	heaph_assert(init_lock_count > 0);
	if (--init_lock_count == 0)
		spinlock_rel(&init_lock);
}

static bool
trace_sym_is_func(const struct symbol *sym, const char *func)
{
	if (sym == NULL || sym->name == NULL)
		return false;
	return strcmp(sym->name, func) == 0;
}

static bool
trace_sym_file_starts_with(const struct symbol *sym, const char *file)
{
	if (sym == NULL || sym->file == NULL)
		return false;
	size_t len = strlen(file);
	return strncmp(sym->file, file, len) == 0;
}

static bool
trace_sym_is_internal(const struct symbol *sym)
{
	if (trace_sym_is_func(sym, "puts"))
		return true;
	if (trace_sym_is_func(sym, "pthread_create"))
		return true;
	if (trace_sym_is_func(sym, "_IO_printf"))
		return true;
	if (trace_sym_is_func(sym, "_IO_puts"))
		return true;
	if (trace_sym_is_func(sym, "_IO_vfscanf"))
		return true;
	if (trace_sym_file_starts_with(sym, "libpthread.so"))
		return true;
	return false;
}

static bool
trace_is_internal(const struct symbol *syms, int count)
{
	for (int i = 0; i < count; ++i) {
		if (trace_sym_is_internal(&syms[i]))
			return true;
	}
	return false;
}

static void
trace_resolve(void *const *addrs, int count, struct symbol *syms)
{
	Dl_info info;
	for (int i = 0; i < count; ++i) {
		struct symbol *s = &syms[i];
		if (dladdr(addrs[i], &info) == 0) {
			heaph_printf("\nHH: dladdr() failure\n");
			memset(s, 0, sizeof(*s));
			continue;
		}
		s->name = info.dli_sname;
		s->file = info.dli_fname;
	}
}

static void
heaph_atexit(void)
{
	// On Mac somehow this atexit handler gets executed 2 times. Have to
	// manually filter out all calls except for the first one.
	if (__atomic_test_and_set(&is_exit_done, __ATOMIC_SEQ_CST))
		return;
	if (report_mode == MODE_QUIET)
		return;
	spinlock_acq(&allocs_lock);
	int64_t count = alloc_count;
	if (count == 0) {
		spinlock_rel(&allocs_lock);
		if (report_mode == MODE_VERBOSE) {
			heaph_printf("\n");
			heaph_printf("HH: found no leaks\n");
			heaph_printf("HH: total allocation count - %llu\n",
			       (long long)alloc_count_total);
		}
		return;
	}
	const struct allocation *a = allocs;
	const int report_limit = 10;
	int report_count = 0;
	int64_t total_count = count;
	uint64_t leak_size = 0;
	struct symbol syms[MAX_BACKTRACE_LEN];
	// People often do not write '\n' in the end of their program. That
	// makes it harder to read HH output unless the latter prepends itself
	// with a line wrap.
	const char *prefix = "\n";
	for (; a != NULL; a = a->next) {
		heaph_assert(count > 0);
		if (a->trace_size == 0) {
			--count;
			continue;
		}
		trace_resolve(a->trace, a->trace_size, syms);
		bool is_internal = trace_is_internal(syms, a->trace_size);
		if (is_internal) {
			--count;
		} else if (report_count < report_limit) {
			heaph_printf("%s", prefix), prefix = "";
			heaph_printf("#### Leak %d (%zu bytes) ####\n",
				     ++report_count, a->size);
			for (int i = 0; i < a->trace_size; ++i)
				heaph_printf("%d - %s\n", i, syms[i].name);
		}
		if (!is_internal)
			leak_size += a->size;
	}
	spinlock_rel(&allocs_lock);

	if (count == 0 && report_mode != MODE_VERBOSE)
		return;
	int64_t suppressed_count = total_count - count;
	heaph_printf("%s", prefix), prefix = "";
	heaph_printf("HH: found %lld leaks (%llu bytes)\n", (long long)count,
		     (long long)leak_size);
	if (suppressed_count != 0) {
		heaph_printf("HH: suppressed %lld internal leaks\n",
			     (long long)suppressed_count);
	}
	if (report_count < count) {
		heaph_printf("HH: only first %d reports are shown\n",
			     report_count);
	}
	heaph_printf("HH: total allocation count - %llu\n",
		     (long long)alloc_count_total);
}

static void
heaph_init(void)
{
	default_malloc = static_malloc;
	default_free = static_free;
	default_calloc = static_calloc;

	void *(*sym_malloc)(size_t) = dlsym(RTLD_NEXT, "malloc");
	void (*sym_free)(void *) = dlsym(RTLD_NEXT, "free");
	void *(*sym_calloc)(size_t, size_t) = dlsym(RTLD_NEXT, "calloc");
	void *(*sym_realloc)(void *, size_t) = dlsym(RTLD_NEXT, "realloc");
	char *(*sym_strdup)(const char *) = dlsym(RTLD_NEXT, "strdup");
	ssize_t (*sym_getline)(char **, size_t *, FILE *) =
		dlsym(RTLD_NEXT, "getline");
	int (*sym_getaddrinfo)(
		const char *, const char *, const struct addrinfo *,
		struct addrinfo **) = dlsym(RTLD_NEXT, "getaddrinfo");
	void (*sym_freeaddrinfo)(struct addrinfo *) =
		dlsym(RTLD_NEXT, "freeaddrinfo");

	default_malloc = sym_malloc;
	default_free = sym_free;
	default_calloc = sym_calloc;
	default_realloc = sym_realloc;
	default_strdup = sym_strdup;
	default_getline = sym_getline;
	default_getaddrinfo = sym_getaddrinfo;
	default_freeaddrinfo = sym_freeaddrinfo;

	const char *hh_report = getenv("HHREPORT");
	if (hh_report != NULL) {
		if (strcmp(hh_report, "v") == 0)
			report_mode = MODE_VERBOSE;
		else if (strcmp(hh_report, "l") == 0)
			report_mode = MODE_LEAKS;
		else if (strcmp(hh_report, "q") == 0)
			report_mode = MODE_QUIET;
	}
	atexit(heaph_atexit);
}

static void
heaph_touch(void)
{
	if (__atomic_load_n(&is_init_done, __ATOMIC_SEQ_CST))
		return;
	if (init_lock_count > 0)
		return;

	heaph_lock();
	heaph_init();
	bool old = __atomic_exchange_n(&is_init_done, true, __ATOMIC_SEQ_CST);
	heaph_unlock();
	heaph_assert(!old);
}

ssize_t
getline(char **linep, size_t *linecapp, FILE *stream)
{
	heaph_touch();
	++depth;
	char *line_old = *linep;
	heaph_assert(!alloc_is_static(line_old));
	ssize_t res = default_getline(linep, linecapp, stream);
	if (line_old == NULL && *linep != NULL) {
		alloc_trace_new(*linep, strlen(*linep) + 1);
	} else if (line_old != NULL && *linep == NULL) {
		heaph_assert(false);
	} else if (line_old != NULL && *linep != NULL && line_old != *linep) {
		alloc_free(line_old);
		alloc_trace_new(*linep, strlen(*linep) + 1);
	} else if (line_old != *linep) {
		heaph_assert(false);
	}
	--depth;
	return res;
}

char *
strdup(const char *ptr)
{
	heaph_touch();
	heaph_assert(!is_exit_done);
	heaph_assert(!alloc_is_static(ptr));
	++depth;
	char *res = default_strdup(ptr);
	if (res != NULL)
		alloc_trace_new(res, strlen(res) + 1);
	--depth;
	return res;
}

void *
malloc(size_t size)
{
	heaph_touch();
	++depth;
	void *res = default_malloc(size);
	if (res != NULL)
		alloc_trace_new(res, size);
	--depth;
	return res;
}

void *
calloc(size_t num, size_t size)
{
	heaph_touch();
	++depth;
	void *res = default_calloc(num, size);
	if (res != NULL)
		alloc_trace_new(res, num * size);
	--depth;
	return res;
}

void *
realloc(void *ptr, size_t size)
{
	heaph_touch();
	++depth;
	if (alloc_is_static(ptr))
		ptr = NULL;
	void *res = default_realloc(ptr, size);
	if (ptr == NULL && res != NULL) {
		alloc_trace_new(res, size);
	} else if (ptr != NULL && res == NULL) {
		alloc_free(ptr);
	} else if (ptr != NULL && res != ptr) {
		alloc_free(ptr);
		alloc_trace_new(res, size);
	}
	--depth;
	return res;
}

void
free(void *ptr)
{
	if (ptr == NULL)
		return;
	if (alloc_is_static(ptr))
		return;
	heaph_touch();
	++depth;
	alloc_free(ptr);
	default_free(ptr);
	--depth;
}

int
getaddrinfo(const char *hostname, const char *servname,
	    const struct addrinfo *hints, struct addrinfo **res)
{
	heaph_touch();
	++depth;
	int rc = default_getaddrinfo(hostname, servname, hints, res);
	if (rc == 0 && *res != NULL)
		alloc_trace_new(*res, sizeof(**res));
	--depth;
	return rc;
}

void
freeaddrinfo(struct addrinfo *ai)
{
	if (ai == NULL)
		return;
	heaph_assert(!alloc_is_static(ai));
	heaph_touch();
	++depth;
	alloc_free(ai);
	default_freeaddrinfo(ai);
	--depth;
}

uint64_t
heaph_get_alloc_count(void)
{
	spinlock_acq(&allocs_lock);
	uint64_t res = alloc_count;
	spinlock_rel(&allocs_lock);
	return res;
}
