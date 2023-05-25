GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant

all: test.o thread_pool.o
	gcc $(GCC_FLAGS) test.o thread_pool.o

test.o: test.c
	gcc $(GCC_FLAGS) -c test.c -o test.o -I ../utils

thread_pool.o: thread_pool.c
	gcc $(GCC_FLAGS) -c thread_pool.c -o thread_pool.o
