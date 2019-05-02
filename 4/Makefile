all: test.o thread_pool.o
	gcc test.o thread_pool.o

test.o: test.c
	gcc -c test.c -o test.o -I ../utils

thread_pool.o: thread_pool.c
	gcc -c thread_pool.c -o thread_pool.o
