#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcoro.h"

/**
 * You can compile and run this code using the commands:
 *
 * $> gcc solution.c libcoro.c
 * $> ./a.out
 */

struct my_context {
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

void
swap(int* a, int* b)
{
    int t = *a;
    *a = *b;
    *b = t;
}
 
int
partition(int arr[], int low, int high)
{
    // Choosing the pivot
    int pivot = arr[high];
 
    // Index of smaller element and indicates
    // the right position of pivot found so far
    int i = (low - 1);
 
    for (int j = low; j <= high - 1; j++) {
 
        // If current element is smaller than the pivot
        if (arr[j] < pivot) {
 
            // Increment index of smaller element
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}
 
void
quick_sort(int arr[], int low, int high)
{
    if (low < high) {
 
    	// pi is partitioning index, arr[p]
        // is now at right place
        int pi = partition(arr, low, high);
 
        // Separately sort elements before
        // partition and after partition
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

void
print_numbers(int* numbers, int* num_items)
{
	for (int i = 0; i < *num_items; i ++)
		printf("%d\n", numbers[i]);
}

static int
get_num_count(char* input)
{
	static const char delim[1] = " ";
	int num_count = 0;

	for (int i = 0; input[i] != '\0'; i++) 
	{
		if (input[i] == *delim)
     	{
          num_count ++;
     	}
	}

	num_count ++;

	return num_count;
}

static int*
parse_input(char* input, int* num_items)
{
	char* token;
	int index = 0;
	 int* numbers = (int*) malloc(*num_items * sizeof(int));
	static const char delim[1] = " ";

	token = strtok(input, delim);

	while( token != NULL )
	{
		numbers[index] = atoi(token);
		token = strtok(NULL, delim);
		index ++;
	}

	return numbers;
}

static char*
read_file(char *file_name)
{
	FILE* fp;
	long num_bytes;

	fp = fopen(file_name, "r");

	fseek(fp, 0, SEEK_END);
	num_bytes = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	char* buffer = (char*) calloc(num_bytes, sizeof(char));

	fread(buffer, sizeof(char), num_bytes, fp);

	fclose(fp);

	return buffer;
}

int
main(int argc, char **argv)
{
	/* Delete these suppressions when start using the args. */
	(void)argc;
	(void)argv;
	/* Initialize our coroutine global cooperative scheduler. */
	coro_sched_init();
	/* Start several coroutines. */
	for (int i = 0; i < 3; ++i) {
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
	while ((c = coro_sched_wait()) != NULL) {
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
	
	char* file_name = "test1.txt";
	char* file_string = malloc(sizeof(char));

	file_string = read_file(file_name);
	printf("%s\n", file_string);

	int num_items = get_num_count(file_string);
	printf("%d\n", num_items);

	int* numbers = parse_input(file_string, &num_items);
	
	print_numbers(numbers, &num_items);

	quick_sort(numbers, 0, num_items);

	print_numbers(numbers, &num_items);

	return 0;
}
