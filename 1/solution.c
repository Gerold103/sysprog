#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcoro.h"
#include <malloc.h>

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

static int parseArraySize(char *str)
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

static int *parseNumbers(char *str, int *arraySize)
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

static int *merge(int numbers[], int left, int middle, int right)
{
	// printf("left: %d, middle: %d, right: %d\n", left, middle, right);
	int leftNumbersSize = middle - left + 1;
	int rightNumbersSize = right - middle;
	// printf("leftNumbersSize: %d, rightNumbersSize: %d\n", leftNumbersSize, rightNumbersSize);
	int leftNumbers[leftNumbersSize], rightNumbers[rightNumbersSize];

	for (int i = 0; i < leftNumbersSize; i++)
		leftNumbers[i] = numbers[left + i];
	for (int j = 0; j < rightNumbersSize; j++)
		rightNumbers[j] = numbers[middle + 1 + j];

	// for (int i = 0; i < leftNumbersSize; i++)
	// 	printf("%d ", leftNumbers[i]);
	// printf("\n");
	// for (int j = 0; j < rightNumbersSize; j++)
	// 	printf("%d ", rightNumbers[j]);
	// printf("\n");

	int i = 0, j = 0;
	int k = left;
	while (i < leftNumbersSize || j < rightNumbersSize)
	{
		// printf("i: %d, j: %d, k: %d\n", i, j, k);
		if (j == rightNumbersSize || (i != leftNumbersSize && leftNumbers[i] <= rightNumbers[j]))
		{
			numbers[k] = leftNumbers[i];
			i++;
		}
		else
		{
			numbers[k] = rightNumbers[j];
			j++;
		}
		k++;
	}
	// for (int z = 0; z < k; z++)
	// 	printf("%d ", numbers[z]);
	// printf("\n-----------\n");
	return numbers;
}

static int *mergeSort(int numbers[], int left, int right)
{
	int *newNumbers = numbers;
	if (left < right)
	{
		int middle = left + (right - left) / 2;

		mergeSort(numbers, left, middle);

		mergeSort(numbers, middle + 1, right);

		newNumbers = merge(numbers, left, middle, right);
	}
	return newNumbers;
}

int main(int argc, char **argv)
{
	char *input = readFile("test.txt");

	char *inputCopy = malloc(sizeof(char) * strlen(input));
	strcpy(inputCopy, input);

	int arraySize = parseArraySize(inputCopy);

	int *numbers = parseNumbers(input, &arraySize);

	int *sortedNumbers = mergeSort(numbers, 0, arraySize - 1);

	for (int i = 0; i < arraySize; i++)
	{
		printf("%d ", sortedNumbers[i]);
	}
	printf("\n");

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
