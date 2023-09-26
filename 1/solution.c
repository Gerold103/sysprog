#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "libcoro.h"

struct my_context {
	char *name;
	long quantum;
	struct sortedArray* all_sorted;
	int* num_remaining_files;
	int total_num_files;
	int index;
};

static struct my_context *
my_context_new(const char *name, int index, long* quantum, struct sortedArray* all_sorted, int* num_remaining_files, int total_num_files)
{
	struct my_context *ctx = malloc(sizeof(*ctx));
	ctx -> name = strdup(name);
	ctx -> quantum = *quantum;
	ctx -> all_sorted = all_sorted;
	ctx ->  num_remaining_files = num_remaining_files;
	ctx -> total_num_files = total_num_files;
	ctx -> index = index;

	return ctx;
}

static void
my_context_delete(struct my_context *ctx)
{
	free(ctx->name);
	free(ctx);
}

// Utils

/**
 * A struct that will be used to hold the sorted arrays 
 * (individual file contents)
 */
struct sortedArray {
	int* arr;
	int length;
	bool sorted;
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

	for(int i = 0; i < content_length; i++)
	{
		fprintf(fp, "%d ", content[i]);
	}

	printf("Success: wrote to file %s\n", file_name);

	fclose(fp);
}

/**
 * A function that appends one array to another given the arrays
 * and their lengths as parameters
 * P.S: make sure first param has enough memory
 */
static void
append_array(int* arr_1, int length_1, int* arr_2, int length_2)
{	
	for (int i = 0; i < length_2; i ++)
		arr_1[i + length_1] = arr_2[i];
}

/**
 * A function that returns the difference in nanoseconds between
 * two timespec structs
 */
static long
get_time_diff_nsec(struct timespec *end, struct timespec *start)
{
	long diff = 0;

	diff += (end->tv_sec - start->tv_sec) * 1e9;
	diff += end->tv_nsec - start->tv_nsec;

	return diff;
}

/**
 * The merge part of merge sort
 */
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

/**
 * Merge sort + time measurements
 */
void
merge_sort(int* arr, int l, int r, long* quantum, struct timespec *start, long *time_taken)
{	

	if (l < r)
	{
		int m = l + (r - l) / 2;

		merge_sort(arr, l, m, quantum, start, time_taken);
		merge_sort(arr, m + 1, r, quantum, start, time_taken);

		merge_and_sort(arr, l, m, r);

		struct timespec *end = malloc(sizeof(*end));
		clock_gettime(CLOCK_MONOTONIC, end);

		long time_diff_nsec = get_time_diff_nsec(end, start);


		free(end);

		if (time_diff_nsec > *quantum)
		{
			*time_taken += time_diff_nsec;
			coro_yield();
			clock_gettime(CLOCK_MONOTONIC, start);
		}
	}
}

// Sorting a single file

/**
 * A function that takes a file name quantum, and a time accumulator
 * opens the file and sorts its content writes 
 * the result back to the file, and to a sortedArray struct given as a parameter
 */
static void
sort_file (char* file_name, long quantum, struct sortedArray* file_sort_res, long* time_taken_nsec)
{
	struct timespec *start = malloc(sizeof(*start));
	clock_gettime(CLOCK_MONOTONIC, start);

	long num_bytes = get_file_num_bytes(file_name);

	char* file_string = (char*) calloc(num_bytes, sizeof(char));

	read_file(file_string, file_name, num_bytes);

	int num_items = get_string_num_count(file_string);

	int* numbers = (int*) malloc(num_items * sizeof(int));

	parse_string_into_array(numbers, file_string);

	free(file_string);

	merge_sort(numbers, 0, num_items, &quantum, start, time_taken_nsec);

	write_file(file_name, numbers, num_items);

	file_sort_res->arr = numbers;
	file_sort_res->length = num_items;
	
	// Adding the last bit of time taken in 
	//case coroutine didn't yield after its last iteration
	struct timespec *end = malloc(sizeof(*end));
	clock_gettime(CLOCK_MONOTONIC, end);
	*time_taken_nsec += get_time_diff_nsec(end, start);

	free(start);
	free(end);
}

/**
 * A single coroutine function implementing a coroutine pool
 */
int
coroutine_func(void *context)
{
	struct coro *this = coro_this();
	struct my_context *ctx = context;
	char* name = ctx->name;
	long quantum = ctx ->quantum;
	struct sortedArray* all_sorted = ctx->all_sorted;
	int total_num_files = ctx->total_num_files;
	int* num_remaining_files = ctx->num_remaining_files;
	long time_taken_nsec = 0;
	char* file_name = malloc(100 * sizeof(char));

	printf("%s: starting\n", name);

	while(true){
		for (int i = 0; i < total_num_files; i++)
		{
			if (!all_sorted[i].sorted)
			{
				all_sorted[i].sorted = true;
				(*num_remaining_files)--;
				sprintf(file_name, "test%d.txt", i + 1);
				printf("%s: working on file %s\n", name, file_name);
				sort_file(file_name, quantum, &all_sorted[i], &time_taken_nsec);
			}
		}

		if(*num_remaining_files <=0) 
			break;	
	}

	printf("%s: switch count %lld\n", name, coro_switch_count(this));
	printf("%s: total working time (in seconds) %lf\n",name, time_taken_nsec / 1e9);

	my_context_delete(ctx);
	free(file_name);
	return 0;
}

int
main(int argc, char **argv)
{
	struct timespec *start_time = malloc(sizeof(*start_time));
	clock_gettime(CLOCK_MONOTONIC, start_time);
	coro_sched_init();

	// Parse args
	int latency = 0;
	int num_coroutines = 0;

	if (argc > 3)
	{
		printf("Error: Too many arguments were provided");
		exit(1);
	}
	else if (argc == 3)
	{
		latency = atoi(argv[1]);
		num_coroutines = atoi(argv[2]);
	}
	else {
		printf("Error: Insufficient arguments were provided. Expected (Latency, No. coroutines)");
		exit(1);
	}

	int num_test_files = count_test_files();
	int remaining_files = num_test_files;
	struct sortedArray* all_sorted = malloc(num_test_files * sizeof(*all_sorted));
	long total_num_items = 0;

	// Covnert to nanoseconds
	long quantum = ((float)latency / num_test_files) * 1e3;
	
	// Start coroutines
	for (int i = 0; i < num_coroutines; ++i) 
	{
		char name[16];
		sprintf(name, "coro_%d", i);
		coro_new(coroutine_func, my_context_new(name, i,&quantum, all_sorted, &remaining_files, num_test_files));
	}

	// End coroutines
	struct coro *c;
	while ((c = coro_sched_wait()) != NULL) {
		printf("Finished %d\n", coro_status(c));
		coro_delete(c);
	}
	
	// Accumulate sort results 
	for (int i = 0; i < num_test_files; ++i) 
	{
		total_num_items += all_sorted[i].length;
	}

	struct sortedArray accumulator = {
		malloc(total_num_items * sizeof(int)), 0, 0
	};

	// Merge sort results
	for (int i = 0; i < num_test_files; i ++)
	{
		
		append_array(accumulator.arr, accumulator.length, all_sorted[i].arr, all_sorted[i].length);

		if (i)
		{	int m = accumulator.length - 1;
			int r = m + all_sorted[i].length;
			merge_and_sort(accumulator.arr, 0, m, r);
		}

		accumulator.length += all_sorted[i].length;
	}

	// Write results to file
	write_file("sum.txt", accumulator.arr, accumulator.length);

	// Calculate total time and free memory
	struct timespec *end_time = malloc(sizeof(*end_time));
	clock_gettime(CLOCK_MONOTONIC, end_time);

	printf("Total program working time (in seconds) %lf\n", get_time_diff_nsec(end_time, start_time) / 1e9);

	for (int i = 0; i < num_test_files; i ++)
	{
		free(all_sorted[i].arr);
	}
	free(all_sorted);
	free(accumulator.arr);
	free(start_time);
	free(end_time);

	return 0;
}
