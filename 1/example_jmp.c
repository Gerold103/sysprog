#include <setjmp.h>
#include <stdio.h>
#include <stdbool.h>

struct task {
        int id;
        jmp_buf env;
        bool is_finished;
	char local_data[128];
};

#define task_count 3
static struct task tasks[task_count];
static int current_task_i = 0;

/**
 * Check not in a function, because setjmp result can not be used
 * after return.
 *
 * Another way how to do not put 'check_resched' after each line -
 * do "define ; check_resched;".
 */
#define check_resched {						\
	/* For an example: just resched after each line. */	\
	int old_i = current_task_i;				\
	current_task_i = (current_task_i + 1) % task_count;	\
	if (setjmp(tasks[old_i].env) == 0)			\
		longjmp(tasks[current_task_i].env, 1); }

static void
my_coroutine()
{
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
        int i;
        for (i = 0; i < task_count; ++i) {
                tasks[i].id = i;
                tasks[i].is_finished = false;
		/* Entry point for new co-coutines. */
                setjmp(tasks[i].env);
        }
        my_coroutine();
        printf("Finished\n");
        return 0;
}
