#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdint.h>

struct worker {
	int fd[2];
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
sorter(struct worker *worker, const char *filename)
{
	close(worker->fd[0]);
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
	write(worker->fd[1], &size, sizeof(size));
	write(worker->fd[1], array, sizeof(int) * size);
	close(worker->fd[1]);
	free(array);
}

void
read_from_pipe(int fd, char *in, int size)
{
	int total = 0;
	int rc = read(fd, in, size);
	while (rc > 0 && size > 0) {
		size -= rc;
		in += rc;
		total += rc;
		rc = read(fd, in, size);
	}
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
		pipe(w->fd);
		w->id = i;
		if (fork() == 0) {
			sorter(w, argv[i + 1]);
			free(workers);
			return 0;
		}
		close(w->fd[1]);
	}
	int total_size = 0;
	w = workers;
	for (int i = 0; i < nfiles; ++i, ++w) {
		read_from_pipe(w->fd[0], (char *) &w->size, sizeof(w->size));
		w->array = malloc(w->size * sizeof(int));
		read_from_pipe(w->fd[0], (char *) w->array,
			       w->size * sizeof(int));
		printf("Got %d numbers from worker %d\n", w->size, w->id);
		close(w->fd[0]);
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
