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

// Merge two subarrays L and M into arr
void
merge(int* arr, int p, int q, int r)
{

	// Create L ← A[p..q] and M ← A[q+1..r]
	int n1 = q - p + 1;
	int n2 = r - q;

	int L[n1], M[n2];

	for (int i = 0; i < n1; i++)
		L[i] = arr[p + i];
	for (int j = 0; j < n2; j++)
		M[j] = arr[q + 1 + j];

	// Maintain current index of sub-arrays and main array
	int i, j, k;
	i = 0;
	j = 0;
	k = p;

	// Until we reach either end of either L or M, pick larger among
	// elements L and M and place them in the correct position at A[p..r]
	while (i < n1 && j < n2)
	{
		if (L[i] <= M[j]) {
			arr[k] = L[i];
			i++;
		} else {
			arr[k] = M[j];
			j++;
		}
		k++;
	}

	// When we run out of elements in either L or M,
	// pick up the remaining elements and put in A[p..r]
	while (i < n1)
	{
		arr[k] = L[i];
		i++;
		k++;
	}

	while (j < n2)
	{
		arr[k] = M[j];
		j++;
		k++;
	}
}

// Divide the array into two subarrays, sort them and merge them
void
merge_sort(int* arr, int l, int r)
{
	if (l < r)
	{
		// m is the point where the array is divided into two subarrays
		int m = l + (r - l) / 2;

		merge_sort(arr, l, m);
		merge_sort(arr, m + 1, r);

		// Merge the sorted subarrays
		merge(arr, l, m, r);
	}
}

void
merge_arrays(int* arr_3, int* arr_1, int size_1, int* arr_2, int size_2)
{
	for (int i = 0; i < size_1; i ++)
		arr_3[i] = arr_1[i];
	
	for (int i = 0; i < size_2; i ++)
		arr_3[i + size_1] = arr_2[i];
}

// static void
// print_numbers(int* numbers, int* num_items)
// {
// 	for (int i = 0; i < *num_items; i ++)
// 		printf("%d\n", numbers[i]);
// }

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

static void
parse_input(int* numbers, char* input)
{
	char* token;
	int index = 0;

	static const char delim[1] = " ";

	token = strtok(input, delim);

	while( token != NULL )
	{
		numbers[index] = atoi(token);
		token = strtok(NULL, delim);
		index ++;
	}
}

static long
get_file_num_bytes(char* file_name)
{
	FILE* fp;
	long num_bytes;

	fp = fopen(file_name, "r");

	if(fp == NULL) {
      printf("Error: could not open file %s\n", file_name);
      exit(1);
   }

	fseek(fp, 0, SEEK_END);
	num_bytes = ftell(fp);

	fclose(fp);

	return num_bytes;
}

static char*
read_file(char* buffer, char *file_name, long num_bytes)
{
	FILE* fp;

	fp = fopen(file_name, "r");

	if(fp == NULL) {
      printf("Error: could not open file %s\n", file_name);
      exit(1);
   }

	fread(buffer, sizeof(char), num_bytes, fp);

	fclose(fp);

	return buffer;
}

static void
write_file(char* file_name, int* content, int content_length)
{
	FILE* fp;

	fp = fopen(file_name, "w");

	if(fp == NULL) {
    	printf("Error: could not open file %s\n", file_name);
    	exit(1);
   	}

	// printf("%d\n%s\n", content_length, file_name);

	for(int i = 0; i < content_length; i++) {
    	fprintf(fp, "%d ", content[i]);
   }

   printf("Success: wrote to file %s\n", file_name);

   fclose(fp);
}

struct sortedArray {
	int* arr;
	int length;
};

static void
sort_file (struct sortedArray* file_sort_res, char* file_name)
{
	long num_bytes = get_file_num_bytes(file_name);

	char* file_string = (char*) calloc(num_bytes, sizeof(char));

	read_file(file_string, file_name, num_bytes);
	// printf("%s\n", file_string);

	int num_items = get_num_count(file_string);
	// printf("%d\n", num_items);

	int* numbers = (int*) malloc(num_items * sizeof(int));

	parse_input(numbers, file_string);
	
	free(file_string);

	// print_numbers(numbers, &num_items);

	merge_sort(numbers, 0, num_items);

	// print_numbers(numbers, &num_items);

	write_file(file_name, numbers, num_items);
	
	file_sort_res->arr = numbers;
	file_sort_res->length = num_items;
}

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

static void
delete_file_data(struct sortedArray* sorted_array, int arr_size)
{
	for (int i = 0; i < arr_size; i ++)
		free(sorted_array[i].arr);
	
	free(sorted_array);

	return;
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
	
	int num_test_files = count_test_files();
	struct sortedArray* numbers = (struct sortedArray*) malloc(sizeof(struct sortedArray*));

	for (int i = 0; i < num_test_files; i ++) {
		char* file_name = malloc(100 * sizeof(char));
		sprintf(file_name, "test%d.txt", i + 1);

		struct sortedArray res;
		sort_file(&res, file_name);
		numbers[i] = res;

		if (i){

			int *cur_length = &numbers[i].length, *prev_length = &numbers[i - 1].length;
			int merge_size = *prev_length + *cur_length;
			int* merge_res =  (int*) malloc(merge_size * sizeof(int));
			merge_arrays(merge_res, numbers[i - 1].arr,  *prev_length, numbers[i].arr, *cur_length);
			
			numbers[i].arr = merge_res;
			numbers[i].length = merge_size;

			int m = *prev_length - 1;

			merge(numbers[i].arr, 0, m, *cur_length - 1);
		}

		if (i == num_test_files - 1)
		{	
			free(file_name);
			free(res.arr);
		}
			
	}

	write_file("sum.txt", numbers[num_test_files - 1].arr, numbers[num_test_files - 1].length);

	delete_file_data(numbers, num_test_files);

	return 0;
}
