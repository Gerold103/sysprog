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

struct my_context
{
	char *name;
	/** ADD HERE YOUR OWN MEMBERS, SUCH AS FILE NAME, WORK TIME, ... */
};

static struct my_context *
my_context_new(const char *name)
{
	struct my_context *ctx = malloc(sizeof(*ctx));
	ctx->name = strdup(name);
	return ctx;
}

static void
my_context_delete(struct my_context *ctx)
{
	free(ctx->name);
	free(ctx);
}

/**
 * A function, called from inside of coroutines recursively. Just to demonstrate
 * the example. You can split your code into multiple functions, that usually
 * helps to keep the individual code blocks simple.
 */
static void
other_function(const char *name, int depth)
{
	printf("%s: entered function, depth = %d\n", name, depth);
	coro_yield();
	if (depth < 3)
		other_function(name, depth + 1);
}

/**
 * Coroutine body. This code is executed by all the coroutines. Here you
 * implement your solution, sort each individual file.
 */
static int
coroutine_func_f(void *context)
{
	/* IMPLEMENT SORTING OF INDIVIDUAL FILES HERE. */

	struct coro *this = coro_this();
	struct my_context *ctx = context;
	char *name = ctx->name;
	printf("Started coroutine %s\n", name);
	printf("%s: switch count %lld\n", name, coro_switch_count(this));
	printf("%s: yield\n", name);
	coro_yield();

	printf("%s: switch count %lld\n", name, coro_switch_count(this));
	printf("%s: yield\n", name);
	coro_yield();

	printf("%s: switch count %lld\n", name, coro_switch_count(this));
	other_function(name, 1);
	printf("%s: switch count after other function %lld\n", name,
		   coro_switch_count(this));

	my_context_delete(ctx);
	/* This will be returned from coro_status(). */
	return 0;
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

static char *readFile(char *filename) // one alloc that is freed in main
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

static int parseArraySize(char *str) // no allocs
{
	int size = 0;
	char *token = strtok(str, " ");
	while (token != NULL)
	{
		token = strtok(NULL, " ");
		size++;
	}

	return size;
}

static int *parseNumbers(char *str, int *arraySize) // one alloc that is freed in main
{
	int i = 0;
	int *numbers = malloc(sizeof(int) * *arraySize);
	char *token = strtok(str, " ");
	while (token != NULL)
	{
		numbers[i] = atoi(token);
		token = strtok(NULL, " ");
		i++;
	}
	return numbers;
}

static int *merge(int *first, int *second, int firstSize, int secondSize)
{
	int *numbers = malloc(sizeof(int) * (firstSize + secondSize));
	int i = 0, j = 0, k = 0;
	while (i < firstSize || j < secondSize)
	{
		if (j == secondSize || (i != firstSize && first[i] <= second[j]))
		{
			numbers[k] = first[i];
			i++;
		}
		else
		{
			numbers[k] = second[j];
			j++;
		}
		k++;
	}
	return numbers;
}

static int *mergeSort(int numbers[], int left, int right)
{
	if (left == right)
	{
		return numbers;
	}
	if (left < right)
	{
		int middle = left + (right - left) / 2;

		int firstSize = middle - left + 1;
		int secondSize = right - middle;
		int *first = malloc(sizeof(int) * firstSize);
		int *second = malloc(sizeof(int) * secondSize);
		for (int i = 0; i < firstSize; i++)
			first[i] = numbers[left + i];
		for (int i = 0; i < secondSize; i++)
			second[i] = numbers[middle + 1 + i];
		int *sortedFirst = mergeSort(first, 0, firstSize - 1);
		int *sortedSecond = mergeSort(second, 0, secondSize - 1);

		free(first);
		free(second);

		int *sorted = merge(sortedFirst, sortedSecond, firstSize, secondSize);

		if (firstSize > 1)
			free(sortedFirst);
		if (secondSize > 1)
			free(sortedSecond);

		return sorted;
	}
	return NULL;
}

static int *mergeSortArray(int *numbers[], int sizes[], int left, int right)
{
	if (left == right)
	{
		return numbers[left];
	}

	if (left < right)
	{
		int middle = left + (right - left) / 2;

		int firstSize = middle - left + 1;
		int secondSize = right - middle;

		int *firstHalf[firstSize];
		int *secondHalf[secondSize];
		int firstHalfSizes[firstSize];
		int secondHalfSizes[secondSize];

		for (int i = 0; i < firstSize; i++)
		{
			// firstHalf[i] = (int *)malloc(sizeof(int) * sizes[left + i]);
			firstHalf[i] = numbers[left + i];
			firstHalfSizes[i] = sizes[left + i];
		}
		for (int i = 0; i < secondSize; i++)
		{
			// secondHalf[i] = (int *)malloc(sizeof(int) * sizes[middle + 1 + i]);
			secondHalf[i] = numbers[middle + 1 + i];
			secondHalfSizes[i] = sizes[middle + 1 + i];
		}
		int *sortedFirst = mergeSortArray(firstHalf, firstHalfSizes, 0, firstSize - 1);
		int *sortedSecond = mergeSortArray(secondHalf, secondHalfSizes, 0, secondSize - 1);

		// for (int i = 0; i < firstSize; i++)
		// {
		// 	if (firstHalfSizes[i] > 1)
		// 		free(firstHalf[i]);
		// }
		// for (int i = 0; i < secondSize; i++)
		// {
		// 	if (secondHalfSizes[i] > 1)
		// 		free(secondHalf[i]);
		// }

		int sz1 = 0, sz2 = 0;
		for (int i = 0; i < firstSize; i++)
		{
			sz1 += firstHalfSizes[i];
		}
		for (int i = 0; i < secondSize; i++)
		{
			sz2 += secondHalfSizes[i];
		}

		int *sorted = merge(sortedFirst, sortedSecond, sz1, sz2);

		if (firstSize > 1)
			free(sortedFirst);
		if (secondSize > 1)
			free(sortedSecond);

		return sorted;
	}

	return NULL; // Handle an invalid case
}

int main(int argc, char **argv)
{
	int *sortedNumbers[argc];
	int sizes[argc];
	int size = 0;
	for (int i = 1; i < argc; i++)
	{
		char *input = readFile(argv[i]);

		char *inputCopy = malloc(sizeof(char) * strlen(input));
		strcpy(inputCopy, input);

		int arraySize = parseArraySize(inputCopy);
		sizes[i - 1] = arraySize;
		size += arraySize;
		printf("arraySize: %d\n", arraySize);

		// sortedNumbers[i - 1] = (int *)malloc(arraySize * sizeof(int));

		int *numbers = parseNumbers(input, &arraySize);

		sortedNumbers[i - 1] = mergeSort(numbers, 0, arraySize - 1);
		free(inputCopy);
		free(input);
		free(numbers);
	}

	int *sortedArray = mergeSortArray(sortedNumbers, sizes, 0, argc - 2);

	writeFile("result.txt", sortedArray, size);
	if (argc > 2)
		free(sortedArray);

	for (int i = 0; i < argc - 1; i++)
	{
		free(sortedNumbers[i]);
	}

	/* Delete these suppressions when start using the args. */
	(void)argc;
	(void)argv;
	/* Initialize our coroutine global cooperative scheduler. */
	coro_sched_init();
	/* Start several coroutines. */
	for (int i = 0; i < 3; ++i)
	{
		/*
		 * The coroutines can take any 'void *' interpretation of which
		 * depends on what you want. Here as an example I give them
		 * some names.
		 */
		char name[16];
		sprintf(name, "coro_%d", i);
		/*
		 * I have to copy the name. Otherwise all the coroutines would
		 * have the same name when they finally start.
		 */
		coro_new(coroutine_func_f, my_context_new(name));
	}
	/* Wait for all the coroutines to end. */
	struct coro *c;
	while ((c = coro_sched_wait()) != NULL)
	{
		/*
		 * Each 'wait' returns a finished coroutine with which you can
		 * do anything you want. Like check its exit status, for
		 * example. Don't forget to free the coroutine afterwards.
		 */
		printf("Finished %d\n", coro_status(c));
		coro_delete(c);
	}
	/* All coroutines have finished. */

	/* IMPLEMENT MERGING OF THE SORTED ARRAYS HERE. */

	return 0;
}
