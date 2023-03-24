#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>

struct worker {
	int *array;
	int size;
};

int
cmp(const void *a, const void *b)
{
	return *(int *)a - *(int *)b;
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
	worker->array = array;
	worker->size = size;
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
	int total_size = 0;
	for (int i = 0; i < nfiles; ++i, ++w) {
		sorter(w, argv[i + 1]);
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
