all: test.o userfs.o
	gcc test.o userfs.o

test.o: test.c
	gcc -c test.c -o test.o

userfs.o: userfs.c
	gcc -c userfs.c -o userfs.o
