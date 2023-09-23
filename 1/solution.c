#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "libcoro.h"

/**
 * You can compile and run this code using the commands:
 *
 * $> gcc solution.c libcoro.c
 * $> ./a.out
 */

struct my_context {
	char *name;
	struct sortedArray* file_sort_res;
	char* file_name;
	/** ADD HERE YOUR OWN MEMBERS, SUCH AS FILE NAME, WORK TIME, ... */
};

static struct my_context *
my_context_new(const char *name, char* file_name, struct sortedArray* file_sort_res)
{
	struct my_context *ctx = malloc(sizeof(*ctx));
	ctx -> name = strdup(name);
	ctx -> file_name = strdup(file_name);
	ctx -> file_sort_res = file_sort_res;
	return ctx;
}

static void
my_context_delete(struct my_context *ctx)
{
	// TODO: Possiblity a problem here with the last prop not being freed
	free(ctx->name);
	free(ctx->file_name);
	free(ctx);
}

/**
 * A function, called from inside of coroutines recursively. Just to demonstrate
 * the example. You can split your code into multiple functions, that usually
 * helps to keep the individual code blocks simple.
 */
// static void
// other_function(const char *name, int depth)
// {
// 	printf("%s: entered function, depth = %d\n", name, depth);
// 	coro_yield();
// 	if (depth < 3)
// 		other_function(name, depth + 1);
// }

/**
 * Coroutine body. This code is executed by all the coroutines. Here you
 * implement your solution, sort each individual file.
 */
// static int
// coroutine_func_f(void *context)
// {
// 	/* IMPLEMENT SORTING OF INDIVIDUAL FILES HERE. */

// 	struct coro *this = coro_this();
// 	struct my_context *ctx = context;
// 	char *name = ctx->name;
// 	printf("Started coroutine %s\n", name);
// 	printf("%s: switch count %lld\n", name, coro_switch_count(this));
// 	printf("%s: yield\n", name);
// 	coro_yield();

// 	printf("%s: switch count %lld\n", name, coro_switch_count(this));
// 	printf("%s: yield\n", name);
// 	coro_yield();

// 	printf("%s: switch count %lld\n", name, coro_switch_count(this));
// 	other_function(name, 1);
// 	printf("%s: switch count after other function %lld\n", name,
// 	       coro_switch_count(this));

// 	my_context_delete(ctx);
// 	/* This will be returned from coro_status(). */
// 	return 0;
// }

// Utils

/**
 * A struct that will be used to hold the sorted arrays 
 * (individual file contents)
 */
struct sortedArray {
	int* arr;
	int length;
};

/**
 * A function that prints an array of numbers to stdout given the array
 * and its length
 */
// static void
// print_num_array(int* arr, int length)
// {
// 	for (int i = 0; i < length; i ++)
// 		printf("%d ", arr[i]);
// }

/**
 * A function that parses a string of whitespace-separated integers
 * into an array of integers given as its first parameter
 * P.S: make sure first param has enough memory
 */
static void
parse_string_into_array(int* numbers, char* input)
{
	char* token;
	int index = 0;

	static const char delim = ' ';

	token = strtok(input, &delim);

	while( token != NULL )
	{
		numbers[index] = atoi(token);
		token = strtok(NULL, &delim);
		index ++;
	}
}

/**
 * A function that returns the number of integers separated by whitespaces
 * in a string given as its parameter
 */
static int
get_string_num_count(char* input)
{
	static const char delim = ' ';
	int num_count = 0;

	for (int i = 0; input[i] != '\0'; i++) 
	{
		if (input[i] == delim)
     	{
          num_count ++;
     	}
	}

	num_count ++;

	return num_count;
}

/**
 * A function that returns the number of bytes present in a file given
 * its name
 */
static long
get_file_num_bytes(char* file_name)
{
	FILE* fp;
	long num_bytes;

	fp = fopen(file_name, "r");

	if(fp == NULL)
	{
		printf("Error: could not open file %s\n", file_name);
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	num_bytes = ftell(fp);

	fclose(fp);

	return num_bytes;
}

/**
 * A function that returns the number of files starting 'test' in cwd
 */
static int
count_test_files() {
	DIR* dir;
	struct dirent* entry;
	int count = 0;

	dir = opendir(".");

	if(dir == NULL) {
		printf("Error: could not open directory\n");
		exit(1);
	}

	while((entry = readdir(dir)) != NULL) {
		if(strncmp(entry->d_name, "test", 4) == 0) {
			count ++;
		}
	}

	closedir(dir);
	return count;
}

/**
 * A function that reads a file given its name and its number of bytes
 * into a buffer given as its first paramter
 */
static char*
read_file(char* buffer, char *file_name, long num_bytes)
{
	FILE* fp;

	fp = fopen(file_name, "r");

	if(fp == NULL)
	{
		printf("Error: could not open file %s\n", file_name);
		exit(1);
	}

	fread(buffer, sizeof(char), num_bytes, fp);

	fclose(fp);

	return buffer;
}

/**
 * A function that writes an array of integers to a file given
 * its name, the array to be written, and its length
 */
static void
write_file(char* file_name, int* content, int content_length)
{
	FILE* fp;

	fp = fopen(file_name, "w");

	if(fp == NULL)
	{
		printf("Error: could not open file %s\n", file_name);
		exit(1);
	}

	// printf("%d\n%s\n", content_length, file_name);

	for(int i = 0; i < content_length; i++)
	{
		fprintf(fp, "%d ", content[i]);
	}

	printf("Success: wrote to file %s\n", file_name);

	fclose(fp);
}

/**
 A function that appends one array to another given the arrays
 * and their lengths as parameters
 * P.S: make sure first param has enough memory
 */
static void
append_array(int* arr_1, int length_1, int* arr_2, int length_2)
{	
	for (int i = 0; i < length_2; i ++)
		arr_1[i + length_1] = arr_2[i];
}

// Merge sort
void
merge_and_sort(int* arr, int l, int m, int r)
{

	int arr1_size = m - l + 1;
	int arr2_size = r - m;
	int arr1[arr1_size], arr2[arr2_size], i = 0, j = 0, k = l;

	for (int i = 0; i < arr1_size; i++)
		arr1[i] = arr[l + i];
	for (int i = 0; i < arr2_size; i++)
		arr2[i] = arr[m + 1 + i];

	while (i < arr1_size && j < arr2_size)
	{
		if (arr1[i] <= arr2[j]) 
		{
			arr[k] = arr1[i];
			i++;
		} else 
		{
			arr[k] = arr2[j];
			j++;
		}
		k++;
	}

	while (i < arr1_size)
	{
		arr[k] = arr1[i];
		i++;
		k++;
	}

	while (j < arr2_size)
	{
		arr[k] = arr2[j];
		j++;
		k++;
	}
}

void
merge_sort(int* arr, int l, int r)
{
	if (l < r)
	{
		int m = l + (r - l) / 2;

		merge_sort(arr, l, m);
		merge_sort(arr, m + 1, r);

		merge_and_sort(arr, l, m, r);
		coro_yield();
	}
}

// Sorting a single file

/**
 * A function that takes a file name as a parameter, opens the file 
 * and sorts in content, writes the result to the file, and do a 
 * sortedArray struct given as a parameter
 */
static int
sort_file (void *context)
{
	// struct coro *this = coro_this();
	struct my_context *ctx = context;
	char* file_name = ctx->file_name;
	struct sortedArray* file_sort_res = ctx->file_sort_res;

	long num_bytes = get_file_num_bytes(file_name);

	char* file_string = (char*) calloc(num_bytes, sizeof(char));

	read_file(file_string, file_name, num_bytes);
	// printf("%s\n", file_string);

	int num_items = get_string_num_count(file_string);
	// printf("%d\n", num_items);

	int* numbers = (int*) malloc(num_items * sizeof(int));

	parse_string_into_array(numbers, file_string);

	free(file_string);

	printf("Hi from %d\n", *ctx->name);

	merge_sort(numbers, 0, num_items);

	printf("Hi from %d\n", *ctx->name);

	write_file(file_name, numbers, num_items);

	file_sort_res->arr = numbers;
	file_sort_res->length = num_items;

	my_context_delete(ctx);

	return 0;
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
	// for (int i = 0; i < 3; ++i) {
	// 	/*
	// 	 * The coroutines can take any 'void *' interpretation of which
	// 	 * depends on what you want. Here as an example I give them
	// 	 * some names.
	// 	 */
	// 	char name[16];
	// 	sprintf(name, "coro_%d", i);
	// 	/*
	// 	 * I have to copy the name. Otherwise all the coroutines would
	// 	 * have the same name when they finally start.
	// 	 */
	// 	coro_new(coroutine_func_f, my_context_new(name));
	// }
	// /* Wait for all the coroutines to end. */
	// struct coro *c;
	// while ((c = coro_sched_wait()) != NULL) {
	// 	/*
	// 	 * Each 'wait' returns a finished coroutine with which you can
	// 	 * do anything you want. Like check its exit status, for
	// 	 * example. Don't forget to free the coroutine afterwards.
	// 	 */
	// 	printf("Finished %d\n", coro_status(c));
	// 	coro_delete(c);
	// }
	/* All coroutines have finished. */

	/* IMPLEMENT MERGING OF THE SORTED ARRAYS HERE. */

	int num_test_files = count_test_files();
	char* file_name = malloc(100 * sizeof(char));
	struct sortedArray* all_sorted = malloc(num_test_files * sizeof(*all_sorted));
	long total_num_items = 0;

	for (int i = 0; i < num_test_files; ++i) {
		char name[16];
		sprintf(name, "coro_%d", i);
		printf("%s\n", name);
		sprintf(file_name, "test%d.txt", i + 1);
		coro_new(sort_file, my_context_new(name, file_name, &all_sorted[i]));
		total_num_items += all_sorted[i].length;
	}

	struct coro *c;
	while ((c = coro_sched_wait()) != NULL) {
		printf("Finished %d\n", coro_status(c));
		coro_delete(c);
	}
	
	// for (int i = 0; i < num_test_files; i ++)
	// {
	// 	sprintf(file_name, "test%d.txt", i + 1);
	// 	sort_file(&all_sorted[i], file_name);
	// 	total_num_items += all_sorted[i].length;
	// }

	struct sortedArray accumulator = {
		malloc(total_num_items * sizeof(int)), 0
	};

	for (int i = 0; i < num_test_files; i ++)
	{
		
		append_array(accumulator.arr, accumulator.length, all_sorted[i].arr, all_sorted[i].length);

		if (i)
		{	int m = accumulator.length - 1;
			int r = m + all_sorted[i].length;
			merge_and_sort(accumulator.arr, 0, m, r);
		}

		accumulator.length = all_sorted[i].length;
	}

	write_file("sum.txt", accumulator.arr, accumulator.length);

	for (int i = 0; i < num_test_files; i ++)
	{
		free(all_sorted[i].arr);
	}
	free(all_sorted);
	free(accumulator.arr);
	free(file_name);

	return 0;
}
