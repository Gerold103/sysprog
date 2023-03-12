#define _GNU_SOURCE
#include "heap_help.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *(*default_malloc)(size_t) = NULL;
void (*default_free)() = NULL;
void *(*default_calloc)(size_t, size_t) = NULL;
void *(*default_realloc)(void *, size_t) = NULL;
char *(*default_strdup)(const char *) = NULL;
ssize_t (*default_getline)(char **, size_t *, FILE *) = NULL;

static uint64_t alloc_count = 0;

ssize_t
getline(char **linep, size_t *linecapp, FILE *stream)
{
	char *line_old = *linep;
	ssize_t res = default_getline(linep, linecapp, stream);
	if (line_old == NULL && *linep != NULL)
		++alloc_count;
	return res;
}

char *
strdup(const char *ptr)
{
	char *res = default_strdup(ptr);
	if (res != NULL)
		++alloc_count;
	return res;
}

void *
malloc(size_t size)
{
	void *res = default_malloc(size);
	if (res != NULL)
		++alloc_count;
	return res;
}

void *
calloc(size_t num, size_t size)
{
	void *res = default_calloc(num, size);
	if (res != NULL)
		++alloc_count;
	return res;
}

void *
realloc(void *ptr, size_t size)
{
	void *res = default_realloc(ptr, size);
	if (ptr == NULL && res != NULL)
		++alloc_count;
	return res;
}

void
free(void *ptr)
{
	if (ptr == NULL)
		return;
	default_free(ptr);
	if (alloc_count == 0) {
		printf("Double-free\n");
		exit(1);
	}
	--alloc_count;
}

void
heaph_init(void)
{
	default_getline = dlsym(RTLD_NEXT, "getline");
	default_strdup = dlsym(RTLD_NEXT, "strdup");
	default_malloc = dlsym(RTLD_NEXT, "malloc");
	default_calloc = dlsym(RTLD_NEXT, "calloc");
	default_realloc = dlsym(RTLD_NEXT, "realloc");
	default_free = dlsym(RTLD_NEXT, "free");
}

uint64_t
heaph_get_alloc_count(void)
{
	return alloc_count;
}
