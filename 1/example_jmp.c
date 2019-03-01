#include <setjmp.h>
#include <stdio.h>
#include <stdbool.h>

/**
 * You can compile and run this example using the commands:
 *
 * $> gcc example_jmp.c
 * $> ./a.out
 */

/**
 * This struct describes one single coroutine. It stores its
 * local variables and a position, where it stands now.
 */
struct task {
        /**
         * Important thing - execution position where that
         * coroutine stopped last time.
         */
        jmp_buf env;
        /**
         * This flag is set when the coroutine has finished its
         * task. It is used to wait until all the coroutines are
         * finished.
         */
        bool is_finished;
	/**
	 * Just local variables for arbitrary usage. You can put
	 * here whatever you want.
	 */
        int id;
	char local_data[128];
};

/**
 * In your code it should be dynamic, not const. Here it is 3 for
 * simplicity.
 */
#define task_count 3
static struct task tasks[task_count];
/**
 * Index of the currently working coroutine. It is used to learn
 * which coroutine should be scheduled next, and to get your
 * current local variables.
 */
static int current_task_i = 0;

/**
 * This macro stops the current coroutine and switches to another
 * one. Check is not in a function, because setjmp result can not
 * be used after return. You should keep it as macros.
 *
 * Below you can see, that 'check_resched' is done after each
 * line. But another way how to do not put 'check_resched' after
 * each line - do "define ; check_resched;". Then each ';' will
 * turn into a context switch.
 */
#define check_resched {						\
	/*							\
	 * For an example: just resched after each line, without\
	 * timeslices.						\
	 */							\
	int old_i = current_task_i;				\
	current_task_i = (current_task_i + 1) % task_count;	\
	if (setjmp(tasks[old_i].env) == 0)			\
		longjmp(tasks[current_task_i].env, 1); }

/**
 * Coroutine body. This code is executed by all the corutines at
 * the same time, but with different struct task. Here you
 * implement your solution.
 */
static void
my_coroutine()
{
	/*
	 * Note - all the data access is done via 'current_task'.
	 * It is not safe to store anything in local variables
	 * here.
	 */
        sprintf(tasks[current_task_i].local_data, "Local data for task_id%d",
        	current_task_i);
	fprintf(stderr, "Before re-schedule: task_id = %d\n", current_task_i);
	check_resched;
	fprintf(stderr, "After first re-schedule, task_id = %d\n",
		current_task_i);
	check_resched;
	fprintf(stderr, "After second re-schedule, task_id = %d\n",
		current_task_i);
	tasks[current_task_i].is_finished = true;
	fprintf(stderr, "This is local data: %s\n",
		tasks[current_task_i].local_data);
	/*
	 * All the things below are just waiting for finish of all
	 * of the coroutines.
	 */
	while (true) {
		bool is_all_finished = true;
		for (int i = 0; i < task_count; ++i) {
			if (! tasks[i].is_finished) {
				fprintf(stderr, "Task_id=%d still active, "\
					"re-scheduling\n", i);
				is_all_finished = false;
				break;
			}
		}
		if (is_all_finished) {
			fprintf(stderr, "No more active tasks to schedule.\n");
			return;
		}
		check_resched;
	}
}

int
main(int argc, char **argv)
{
        for (int i = 0; i < task_count; ++i) {
                tasks[i].id = i;
                tasks[i].is_finished = false;
		/* Entry point for new co-coutines. */
                setjmp(tasks[i].env);
        }
        my_coroutine();
        printf("Finished\n");
        return 0;
}
