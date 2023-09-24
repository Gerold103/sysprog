GCC_FLAGS_MEM_LEAK = -Wextra -Werror -Wall -Wno-gnu-folding-constant -ldl -rdynamic
GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant

all: libcoro.c solution.c
	gcc $(GCC_FLAGS) libcoro.c solution.c

all_mem_leak: libcoro.c solution.c
	gcc $(GCC_FLAGS_MEM_LEAK) libcoro.c solution.c ../utils/heap_help/heap_help.c

clean_out:
	rm -f *.out

clean_txt:
	rm -f test.txt test1.txt test2.txt test3.txt test4.txt test5.txt test6.txt result.txt

clean_all: clean_out clean_txt

run: all
	python3 generator.py -f test1.txt -c 50000 -m 100000
	python3 generator.py -f test2.txt -c 50000 -m 100000
	python3 generator.py -f test3.txt -c 50000 -m 100000
	python3 generator.py -f test4.txt -c 50000 -m 100000
	python3 generator.py -f test5.txt -c 50000 -m 100000
	python3 generator.py -f test6.txt -c 50000 -m 100000
	./a.out 100 test1.txt test2.txt test3.txt test5.txt test5.txt test6.txt  

run_mem_leak: all_mem_leak
	python3 generator.py -f test1.txt -c 10000 -m 10000
	python3 generator.py -f test2.txt -c 10000 -m 10000
	python3 generator.py -f test3.txt -c 10000 -m 10000
	./a.out 100 test1.txt test2.txt test3.txt  

part: all
	python3 generator.py -f test.txt -c 10000 -m 10000
	./a.out test.txt

part_mem_leak: all_mem_leak
	python3 generator.py -f test.txt -c 10000 -m 10000
	./a.out test.txt

test: 
	python3 checker.py -f result.txt