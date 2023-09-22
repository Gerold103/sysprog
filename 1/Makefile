GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant
LEAK_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant -ldl -rdynamic
RELAXED_FLAGS = -Wextra -Wall -Wno-gnu-folding-constant

all: libcoro.c solution.c
	./generator_generator.sh 5
	gcc $(GCC_FLAGS) libcoro.c solution.c

leaks: libcoro.c solution.c ../utils/heap_help/heap_help.c
	./generator_generator.sh 5
	gcc $(LEAK_FLAGS) libcoro.c solution.c ../utils/heap_help/heap_help.c

relaxed: libcoro.c solution.c
	./generator_generator.sh 5
	gcc $(RELAXED_FLAGS) libcoro.c solution.c

clean:
	rm a.out
