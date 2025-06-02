#include "parser.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

static void
execute_command_line(const struct command_line *line)
{
	/* REPLACE THIS CODE WITH ACTUAL COMMAND EXECUTION */

	assert(line != NULL);
	printf("================================\n");
	printf("Command line:\n");
	printf("Is background: %d\n", (int)line->is_background);
	printf("Output: ");
	if (line->out_type == OUTPUT_TYPE_STDOUT) {
		printf("stdout\n");
	} else if (line->out_type == OUTPUT_TYPE_FILE_NEW) {
		printf("new file - \"%s\"\n", line->out_file);
	} else if (line->out_type == OUTPUT_TYPE_FILE_APPEND) {
		printf("append file - \"%s\"\n", line->out_file);
	} else {
		assert(false);
	}
	printf("Expressions:\n");
	const struct expr *e = line->head;
	while (e != NULL) {
		if (e->type == EXPR_TYPE_COMMAND) {
			printf("\tCommand: %s", e->cmd.exe);
			for (uint32_t i = 0; i < e->cmd.arg_count; ++i)
				printf(" %s", e->cmd.args[i]);
			printf("\n");
		} else if (e->type == EXPR_TYPE_PIPE) {
			printf("\tPIPE\n");
		} else if (e->type == EXPR_TYPE_AND) {
			printf("\tAND\n");
		} else if (e->type == EXPR_TYPE_OR) {
			printf("\tOR\n");
		} else {
			assert(false);
		}
		e = e->next;
	}
}

int
main(void)
{
	const size_t buf_size = 1024;
	char buf[buf_size];
	int rc;
	struct parser *p = parser_new();
	while ((rc = read(STDIN_FILENO, buf, buf_size)) > 0) {
		parser_feed(p, buf, rc);
		struct command_line *line = NULL;
		while (true) {
			enum parser_error err = parser_pop_next(p, &line);
			if (err == PARSER_ERR_NONE && line == NULL)
				break;
			if (err != PARSER_ERR_NONE) {
				printf("Error: %d\n", (int)err);
				continue;
			}
			execute_command_line(line);
			command_line_delete(line);
		}
	}
	parser_delete(p);
	return 0;
}
