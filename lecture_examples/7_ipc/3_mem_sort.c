#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sched.h>

#define MEM_SIZE 65536
#define IS_READABLE 0
#define IS_WRITABLE 1
#define MEM_META_SIZE 2

struct worker {
	char *mem;
	int *array;
	int size;
	int id;
};

int
cmp(const void *a, const void *b)
{
	return *(int *)a - *(int *)b;
}

void
write_to_shared_mem(char *mem, const char *src, int size)
{
	volatile char *is_readable = &mem[IS_READABLE];
	volatile char *is_writable = &mem[IS_WRITABLE];
	mem += MEM_META_SIZE;
	int mem_size = MEM_SIZE - MEM_META_SIZE;
	int saved_size = mem_size;
	char *saved_mem = mem;
	while (1) {
		while (! __atomic_load_n(is_writable, __ATOMIC_ACQUIRE))
			sched_yield();
		int to_copy = mem_size > size ? size : mem_size;
		memcpy(mem, src, to_copy);
		size -= to_copy;
		mem_size -= to_copy;
		mem += to_copy;
		src += to_copy;

		__atomic_store_n(is_writable, 0, __ATOMIC_RELEASE);
		__atomic_store_n(is_readable, 1, __ATOMIC_RELEASE);
		if (size == 0)
			break;
		mem = saved_mem;
		mem_size = saved_size;
	}
}

void
read_from_shared_mem(char *mem, char *dst, int size)
{
	volatile char *is_readable = &mem[IS_READABLE];
	volatile char *is_writable = &mem[IS_WRITABLE];
	mem += MEM_META_SIZE;
	int mem_size = MEM_SIZE - MEM_META_SIZE;
	int saved_size = mem_size;
	char *saved_mem = mem;
	while (1) {
		while (! __atomic_load_n(is_readable, __ATOMIC_ACQUIRE))
			sched_yield();
		int to_copy = mem_size > size ? size : mem_size;
		memcpy(dst, mem, to_copy);
		size -= to_copy;
		mem_size -= to_copy;
		mem += to_copy;
		dst += to_copy;

		__atomic_store_n(is_readable, 0, __ATOMIC_RELEASE);
		__atomic_store_n(is_writable, 1, __ATOMIC_RELEASE);
		if (size == 0)
			break;
		mem = saved_mem;
		mem_size = saved_size;
	}
}

void
sorter(struct worker *worker, const char *filename)
{
	FILE *file = fopen(filename, "r");
	int size = 0;
	int capacity = 1024;
	int *array = malloc(capacity * sizeof(int));
	while (fscanf(file, "%d", &array[size]) > 0) {
		++size;
		if (size == capacity) {
			capacity *= 2;
			array = realloc(array, capacity * sizeof(int));
		}
	}
	qsort(array, size, sizeof(int), cmp);
	fclose(file);
	printf("Worker %d sorted %d numbers\n", worker->id, size);
	write_to_shared_mem(worker->mem, (char *) &size, sizeof(size));
	write_to_shared_mem(worker->mem, (char *) array, sizeof(int) * size);
	free(array);
}

int
main(int argc, const char **argv)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint64_t start_ns = ts.tv_sec * 1000000000 + ts.tv_nsec;
	int nfiles = argc - 1;
	struct worker *workers = malloc(sizeof(struct worker) * nfiles);
	struct worker *w = workers;
	for (int i = 0; i < nfiles; ++i, ++w) {
		w->id = i;
		w->mem = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE,
			      MAP_ANON | MAP_SHARED, -1, 0);
		w->mem[IS_READABLE] = 0;
		w->mem[IS_WRITABLE] = 1;
		if (fork() == 0) {
			sorter(w, argv[i + 1]);
			free(workers);
			return 0;
		}
	}
	int total_size = 0;
	w = workers;
	for (int i = 0; i < nfiles; ++i, ++w) {
		read_from_shared_mem(w->mem, (char *) &w->size,
				     sizeof(w->size));
		w->array = malloc(w->size * sizeof(int));
		read_from_shared_mem(w->mem, (char *) w->array,
				     w->size * sizeof(int));
		printf("Got %d numbers from worker %d\n", w->size, w->id);
		wait(NULL);
		total_size += w->size;
	}
	int *total_array = malloc(total_size * sizeof(int));
	int *pos = total_array;
	w = workers;
	for (int i = 0; i < nfiles; ++i, ++w) {
		memcpy(pos, w->array, w->size * sizeof(int));
		pos += w->size;
	}
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint64_t end_ns = ts.tv_sec * 1000000000 + ts.tv_nsec;
	double sec = (end_ns - start_ns) / 1000000000.0;
	printf("presort time = %lfs\n", sec);
	return 0;
}
