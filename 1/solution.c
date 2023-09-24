// #define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcoro.h"
#include <malloc.h>
#include "../utils/heap_help/heap_help.h"
#include <time.h>

static char *readFile(char *filename);						   // read file and return string
static int *parseNumbers(char *str);						   // parse string to array of numbers
static void writeFile(char *filename, int *numbers, int size); // write array of numbers to file
/*MergeSort functions*/
static int *merge(int *a, int *b);					 // merge two sorted arrays
static int *mergeSort(int *numbers, void *context);	 // sort array using merge sort
static int *mergeSortArrays(int **arrays, int size); // sort array of sorted arrays using merge sort

struct my_context
{
	char *name;			   // filename
	int **sorted;		   // pointer to sorted array of the file
	double quantum;		   // T/N , stored in microseconds
	struct timespec start; // start time of the coroutine
};

static struct my_context *
my_context_new(const char *name, int **sorted, double quantum)
{
	struct my_context *ctx = malloc(sizeof(*ctx));
	ctx->name = strdup(name);
	ctx->sorted = sorted;
	ctx->quantum = quantum;

	return ctx;
}

static void
my_context_delete(struct my_context *ctx)
{
	free(ctx->name);
	free(ctx);
}

static int
coroutine_func_f(void *context)
{
	// struct coro *this = coro_this();
	struct my_context *ctx = context;

	// start timer
	clock_gettime(CLOCK_MONOTONIC, &ctx->start);
	int **sorted = ctx->sorted;

	char *input = readFile(ctx->name);
	int *numbers = parseNumbers(input);
	sorted[0] = mergeSort(numbers, ctx);

	free(numbers);
	free(input);
	my_context_delete(ctx);
	// printf("coroutine switch count: %lld\n", coro_switch_count(this));
	return 0;
}

int main(int argc, char **argv)
{
	int T = atof(argv[1]);
	int *sortedFiles[argc - 1];
	coro_sched_init();

	for (int i = 2; i < argc; i++)
	{
		char *filename = argv[i];
		coro_new(coroutine_func_f, my_context_new(filename, &sortedFiles[i - 2], T / (argc - 2.0)));
	}

	struct coro *c;
	while ((c = coro_sched_wait()) != NULL)
	{
		coro_delete(c);
	}

	int *sortedArr = mergeSortArrays(sortedFiles, argc - 2);
	int *writeArr = malloc(sizeof(int) * (sortedArr[0]));
	for (int i = 0; i < sortedArr[0]; i++)
	{
		writeArr[i] = sortedArr[i + 1];
	}
	writeFile("result.txt", writeArr, sortedArr[0]);

	if (argc > 3)
		free(sortedArr);
	free(writeArr);
	for (int i = 2; i < argc; i++)
	{
		free(sortedFiles[i - 2]);
	}

	return 0;
}

static char *readFile(char *filename)
{
	FILE *fptr = fopen(filename, "r");

	fseek(fptr, 0L, SEEK_END);
	int size = ftell(fptr) + 1;

	rewind(fptr);

	char *fileInput = malloc(sizeof(char) * size);
	fgets(fileInput, size, fptr);
	fclose(fptr);

	return fileInput;
}

static int *parseNumbers(char *str)
{
	// get size of array
	char *strCopy = malloc(sizeof(char) * strlen(str));
	strcpy(strCopy, str);
	int size = 0;
	char *token1 = strtok(strCopy, " ");
	while (token1 != NULL)
	{
		token1 = strtok(NULL, " ");
		size++;
	}

	free(strCopy);

	int i = 1;
	int *numbers = malloc(sizeof(int) * (size + 1));
	numbers[0] = size;
	char *token2 = strtok(str, " ");
	while (token2 != NULL)
	{
		numbers[i] = atoi(token2);
		token2 = strtok(NULL, " ");
		i++;
	}
	return numbers;
}

static void writeFile(char *filename, int *numbers, int size)
{
	FILE *fptr = fopen(filename, "w");
	for (int i = 0; i < size; i++)
	{
		fprintf(fptr, "%d ", numbers[i]);
	}
	fclose(fptr);
}

static int *merge(int *a, int *b)
{
	int sizeA = a[0];
	int sizeB = b[0];
	int *merged = malloc(sizeof(int) * (sizeA + sizeB + 1));
	merged[0] = sizeA + sizeB;
	int i = 1;
	int j = 1;
	int k = 1;
	while (i <= sizeA && j <= sizeB)
	{
		if (a[i] < b[j])
		{
			merged[k] = a[i];
			i++;
		}
		else
		{
			merged[k] = b[j];
			j++;
		}
		k++;
	}
	while (i <= sizeA)
	{
		merged[k] = a[i];
		i++;
		k++;
	}
	while (j <= sizeB)
	{
		merged[k] = b[j];
		j++;
		k++;
	}
	return merged;
}

static int *mergeSort(int *numbers, void *context)
{
	struct my_context *ctx = context;
	int size = numbers[0];
	if (size == 1)
	{
		return numbers;
	}
	else
	{
		int mid = size / 2;
		int *left = malloc(sizeof(int) * (mid + 1));
		int *right = malloc(sizeof(int) * (size - mid + 1));
		left[0] = mid;
		right[0] = size - mid;
		for (int i = 1; i <= mid; i++)
		{
			left[i] = numbers[i];
		}
		for (int i = mid + 1; i <= size; i++)
		{
			right[i - mid] = numbers[i];
		}
		int *sortedLeft = mergeSort(left, ctx);
		int *sortedRight = mergeSort(right, ctx);
		int *sorted = merge(sortedLeft, sortedRight);

		struct timespec end;
		clock_gettime(CLOCK_MONOTONIC, &end);
		double x = 1000000.0 * end.tv_sec + 1e-3 * end.tv_nsec - (1000000.0 * ctx->start.tv_sec + 1e-3 * ctx->start.tv_nsec);
		if (x > ctx->quantum) // check if time elapsed is greater than quantum
		{
			// yield and restart timer
			coro_yield();
			clock_gettime(CLOCK_MONOTONIC, &ctx->start);
		}

		free(left);
		free(right);
		if (mid > 1)
			free(sortedLeft);
		if (size - mid > 1)
			free(sortedRight);
		return sorted;
	}
	return NULL;
}

static int *mergeSortArrays(int **arrays, int size)
{
	if (size == 1)
	{
		return arrays[0];
	}
	else
	{
		int mid = size / 2;
		int **left = malloc(sizeof(int *) * mid);
		int **right = malloc(sizeof(int *) * (size - mid));
		for (int i = 0; i < mid; i++)
		{
			left[i] = arrays[i];
		}
		for (int i = mid; i < size; i++)
		{
			right[i - mid] = arrays[i];
		}
		int *sortedLeft = mergeSortArrays(left, mid);
		int *sortedRight = mergeSortArrays(right, size - mid);
		int *sorted = merge(sortedLeft, sortedRight);
		free(left);
		free(right);
		if (mid > 1)
			free(sortedLeft);
		if (size - mid > 1)
			free(sortedRight);
		return sorted;
	}
	return NULL;
}