#include "parser.h"

#include <assert.h>
#include <iostream>
#include <unistd.h>

static void
execute_command_line(const command_line &line)
{
	/* REPLACE THIS CODE WITH ACTUAL COMMAND EXECUTION */

	std::cout << "================================\n";
	std::cout << "Command line:\n";
	std::cout << "Is background: " << line.is_background << '\n';
	std::cout << "Output: ";
	if (line.out_type == OUTPUT_TYPE_STDOUT) {
		std::cout << "stdout\n";
	} else if (line.out_type == OUTPUT_TYPE_FILE_NEW) {
		std::cout << "new file - \"" << line.out_file.value() << "\"\n";
	} else if (line.out_type == OUTPUT_TYPE_FILE_APPEND) {
		std::cout << "append file - \"" << line.out_file.value() << "\"\n";
	} else {
		assert(false);
	}
	std::cout << "Expressions:\n";
	for (const expr &e : line.exprs) {
		if (e.type == EXPR_TYPE_COMMAND) {
			std::cout << "\tCommand: " << e.cmd.exe;
			for (char *arg : e.cmd.args)
				std::cout << ' ' << arg;
			std::cout << '\n';
		} else if (e.type == EXPR_TYPE_PIPE) {
			std::cout << "\tPIPE\n";
		} else if (e.type == EXPR_TYPE_AND) {
			std::cout << "\tAND\n";
		} else if (e.type == EXPR_TYPE_OR) {
			std::cout << "\tOR\n";
		} else {
			assert(false);
		}
	}
}

int
main(void)
{
	const size_t buf_size = 1024;
	char buf[buf_size];
	int rc;
	parser p;
	while ((rc = read(STDIN_FILENO, buf, buf_size)) > 0) {
		p.feed(buf, rc);
		command_line *line = nullptr;
		while (true) {
			parser_error err = p.pop_next(line);
			if (err == PARSER_ERR_NONE && line == nullptr)
				break;
			if (err != PARSER_ERR_NONE) {
				std::cout << "Error: " << err << "\n";
				continue;
			}
			execute_command_line(*line);
			delete line;
		}
	}
	return 0;
}
