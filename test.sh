#!/bin/bash

# Build all *.c files. And then + leaks.

cp src/test.c ./solution/
cd solution
make
if ./test; then
	echo "Passed!"
else
	echo "Fail!"
fi
