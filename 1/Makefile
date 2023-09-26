GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant
LEAK_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant -ldl -rdynamic
RELAXED_FLAGS = -Wextra -Wall -Wno-gnu-folding-constant

all: libcoro.c solution.c
	./generator_generator.sh 6
	gcc $(GCC_FLAGS) libcoro.c solution.c

leaks: libcoro.c solution.c ../utils/heap_help/heap_help.c
	./generator_generator.sh 6
	gcc $(LEAK_FLAGS) libcoro.c solution.c ../utils/heap_help/heap_help.c

debug: libcoro.c solution.c
	./generator_generator.sh 6
	gcc $(RELAXED_FLAGS) libcoro.c solution.c -g

relaxed: libcoro.c solution.c
	./generator_generator.sh 6
	gcc $(RELAXED_FLAGS) libcoro.c solution.c

test:
	./checker_checker.sh

clean:
	rm -f a.out
	find  . -name 'test*' -exec rm {} \;
	rm -f sum.txt
