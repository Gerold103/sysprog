#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcoro.h"
#include <malloc.h>
#include "../utils/heap_help/heap_help.h"

/**
 * You can compile and run this code using the commands:
 *
 * $> gcc solution.c libcoro.c
 * $> ./a.out
 */

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

	free(strCopy); // free char *strCopy

	// parse numbers
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

static int *mergeSort(int *numbers)
{
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
		int *sortedLeft = mergeSort(left);
		int *sortedRight = mergeSort(right);
		int *sorted = merge(sortedLeft, sortedRight);
		coro_yield();
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

struct my_context
{
	int id;
	char *name;
	int **sorted;
};

static struct my_context *
my_context_new(const char *name, int id, int **sorted)
{
	struct my_context *ctx = malloc(sizeof(*ctx));
	ctx->name = strdup(name);
	ctx->sorted = sorted;
	ctx->id = id;

	return ctx;
}

static void
my_context_delete(struct my_context *ctx)
{
	free(ctx->name);
	free(ctx);
}

/**
 * Coroutine body. This code is executed by all the coroutines. Here you
 * implement your solution, sort each individual file.
 */
static int
coroutine_func_f(void *context)
{
	// struct coro *this = coro_this();
	struct my_context *ctx = context;
	int **sorted = ctx->sorted;

	char *input = readFile(ctx->name);

	int *numbers = parseNumbers(input);

	sorted[0] = mergeSort(numbers);

	free(numbers);

	free(input);

	my_context_delete(ctx);
	return 0;
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

int main(int argc, char **argv)
{
	int *sortedFiles[argc];
	coro_sched_init();

	for (int i = 1; i < argc; i++)
	{
		char *filename = argv[i];
		coro_new(coroutine_func_f, my_context_new(filename, i - 1, &sortedFiles[i - 1]));
	}

	/* Wait for all the coroutines to end. */
	struct coro *c;
	while ((c = coro_sched_wait()) != NULL)
	{
		printf("Finished %d\n", coro_status(c));
		coro_delete(c);
	}

	int *sortedArr = mergeSortArrays(sortedFiles, argc - 1);
	int *writeArr = malloc(sizeof(int) * (sortedArr[0]));
	for (int i = 0; i < sortedArr[0]; i++)
	{
		writeArr[i] = sortedArr[i + 1];
	}
	writeFile("result.txt", writeArr, sortedArr[0]);

	if (argc > 2)
		free(sortedArr);
	free(writeArr);
	for (int i = 1; i < argc; i++)
	{
		free(sortedFiles[i - 1]);
	}

	return 0;
}
