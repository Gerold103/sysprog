#include "parser.h"

#include "unit.h"

#include <string.h>

static void
test_one_word(void)
{
	unit_test_start();
	struct parser *p = parser_new();
	struct command_line *line = NULL;

	parser_feed(p, "ls\n", 3);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_STDOUT, "out type");
	unit_check(line->out_file == NULL, "out file");
	unit_check(!line->is_background, "is background");
	unit_check(line->head == line->tail && line->head != NULL, "one expr");
	struct expr *e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "ls") == 0, "exe");
	unit_check(e->cmd.arg_count == 0, "arg count");
	unit_check(e->next == NULL, "no next exprs");
	command_line_delete(line);

	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line == NULL, "no more lines yet");

	unit_msg("Feed next command");
	parser_feed(p, "   pwd \n", 8);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "pwd") == 0, "exe");
	unit_check(e->cmd.arg_count == 0, "arg count");
	unit_check(e->next == NULL, "no next exprs");
	command_line_delete(line);

	parser_delete(p);
	unit_test_finish();
}

static void
test_incomplete(void)
{
	unit_test_start();
	struct parser *p = parser_new();
	struct command_line *line = NULL;

	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line == NULL, "no more lines yet");

	parser_feed(p, "l", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line == NULL, "no more lines yet");

	parser_feed(p, "s", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line == NULL, "no more lines yet");

	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->head->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(line->head->cmd.exe, "ls") == 0, "exe");
	command_line_delete(line);

	parser_delete(p);
	unit_test_finish();
}

static void
test_two_words(void)
{
	unit_test_start();
	struct parser *p = parser_new();
	struct command_line *line = NULL;

	const char *str = "mkdir ../testdir";
	uint32_t len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	str = "   \t\r  \n";
	parser_feed(p, str, strlen(str));
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	struct expr *e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "mkdir") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "../testdir") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	unit_msg("Quoted argument");
	str = "touch \"my file with whitespaces in name.txt\"";
	len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "touch") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0],
		"my file with whitespaces in name.txt") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	parser_delete(p);
	unit_test_finish();
}

static void
test_escape_in_string(void)
{
	unit_test_start();
	struct parser *p = parser_new();
	struct command_line *line = NULL;

	/* echo '123 456 \" str \"' */
	const char *str = "echo '123 456 \\\" str \\\"'";
	uint32_t len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	struct expr *e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0],
		"123 456 \\\" str \\\"") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	/* echo "test 'test'' \\" */
	str = "echo \"test 'test'' \\\\\"";
	len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "test 'test'' \\") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	unit_msg("Complex string");
	/*
	 * printf "import time\n\
	 * time.sleep(0.1)\n\
	 * f = open('test.txt', 'a')\n\
	 * f.write('Text\\\\n')\n\
	 * f.close()\n" > test.py
	 */
	str = "printf \"import time\\n\\\n"
		"time.sleep(0.1)\\n\\\n"
		"f = open('test.txt', 'a')\\n\\\n"
		"f.write('Text\\\\\\\\n')\\n\\\n"
		"f.close()\\n\" > test.py";
	len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_FILE_NEW, "out type");
	unit_check(strcmp(line->out_file, "test.py") == 0, "out file");
	e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "printf") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0],
		"import time\\n"
		"time.sleep(0.1)\\n"
		"f = open('test.txt', 'a')\\n"
		"f.write('Text\\\\n')\\n"
		"f.close()\\n") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	parser_delete(p);
	unit_test_finish();
}

static void
test_output_redirect(void)
{
	unit_test_start();
	struct parser *p = parser_new();
	struct command_line *line = NULL;

	/* echo '123 456 \" str \"' > "my file with whitespaces in name.txt" */
	const char *str = "echo '123 456 \\\" str \\\"' > \"my file with whitespaces in name.txt\"";
	uint32_t len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_FILE_NEW, "out type");
	unit_check(strcmp(line->out_file,
		"my file with whitespaces in name.txt") == 0, "out file");
	unit_check(!line->is_background, "not background");
	struct expr *e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0],
		"123 456 \\\" str \\\"") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	unit_msg("Append to file");
	/* echo "test" >> "my file with whitespaces in name.txt" */
	str = "echo \"test\" >> \"my file with whitespaces in name.txt\"";
	len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_FILE_APPEND, "out type");
	unit_check(strcmp(line->out_file,
		"my file with whitespaces in name.txt") == 0, "out file");
	unit_check(!line->is_background, "not background");
	e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "test") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	unit_msg("No spaces");
	str = "echo \"4\">file";
	len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_FILE_NEW, "out type");
	unit_check(strcmp(line->out_file, "file") == 0, "out file");
	unit_check(!line->is_background, "not background");
	e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "4") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	parser_delete(p);
	unit_test_finish();
}

static void
test_escape_outside_of_string(void)
{
	unit_test_start();
	struct parser *p = parser_new();
	struct command_line *line = NULL;

	/* cat my\ file\ with\ whitespaces\ in\ name.txt */
	const char *str = "cat my\\ file\\ with\\ whitespaces\\ in\\ name.txt";
	uint32_t len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_STDOUT, "out type");
	unit_check(!line->is_background, "not background");
	struct expr *e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "cat") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0],
		"my file with whitespaces in name.txt") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	unit_msg("Escape new line");
	/*
	 * echo 123\
	 * 456
	 */
	str = "echo 123\\\n456";
	len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_STDOUT, "out type");
	unit_check(!line->is_background, "not background");
	e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "123456") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	/*
	 * echo 123\
	 * 456\
	 * | grep 2
	 */
	str = "echo 123\\\n456\\\n| grep 2";
	len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_STDOUT, "out type");
	unit_check(!line->is_background, "not background");
	e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "123456") == 0, "arg[0]");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_PIPE, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "grep") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "2") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	parser_delete(p);
	unit_test_finish();
}

static void
test_pipe(void)
{
	unit_test_start();
	struct parser *p = parser_new();
	struct command_line *line = NULL;

	const char *str = "echo 100|grep 100";
	uint32_t len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_STDOUT, "out type");
	unit_check(!line->is_background, "not background");
	struct expr *e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "100") == 0, "arg[0]");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_PIPE, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "grep") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "100") == 0, "arg[0]");

	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	unit_msg("Multiple pipes");
	str = "echo 'source string' | sed 's/source/destination/g' | sed 's/string/value/g'";
	len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_STDOUT, "out type");
	unit_check(!line->is_background, "not background");
	e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "source string") == 0, "arg[0]");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_PIPE, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "sed") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0],
		"s/source/destination/g") == 0, "arg[0]");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_PIPE, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "sed") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "s/string/value/g") == 0, "arg[0]");

	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	unit_msg("Multiple args and pipes");
	str = "yes bigdata | head -n 100000 | wc -l | tr -d [:blank:]";
	len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_STDOUT, "out type");
	unit_check(!line->is_background, "not background");
	e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "yes") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "bigdata") == 0, "arg[0]");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_PIPE, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "head") == 0, "exe");
	unit_check(e->cmd.arg_count == 2, "arg count");
	unit_check(strcmp(e->cmd.args[0], "-n") == 0, "arg[0]");
	unit_check(strcmp(e->cmd.args[1], "100000") == 0, "arg[1]");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_PIPE, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "wc") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "-l") == 0, "arg[0]");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_PIPE, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "tr") == 0, "exe");
	unit_check(e->cmd.arg_count == 2, "arg count");
	unit_check(strcmp(e->cmd.args[0], "-d") == 0, "arg[0]");
	unit_check(strcmp(e->cmd.args[1], "[:blank:]") == 0, "arg[1]");

	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	parser_delete(p);
	unit_test_finish();
}

static void
test_comments(void)
{
	unit_test_start();
	struct parser *p = parser_new();
	struct command_line *line = NULL;

	const char *str = "echo 100 # comment ' ' \\ \\";
	uint32_t len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_STDOUT, "out type");
	unit_check(!line->is_background, "not background");
	struct expr *e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "100") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	str = " # empty line, only comment";
	len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line == NULL, "comment lines are skipped");

	str = "grep 300 400 # comm";
	len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_STDOUT, "out type");
	unit_check(!line->is_background, "not background");
	e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "grep") == 0, "exe");
	unit_check(e->cmd.arg_count == 2, "arg count");
	unit_check(strcmp(e->cmd.args[0], "300") == 0, "arg[0]");
	unit_check(strcmp(e->cmd.args[1], "400") == 0, "arg[1]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	parser_delete(p);
	unit_test_finish();
}

static void
test_multiline_string(void)
{
	unit_test_start();
	struct parser *p = parser_new();
	struct command_line *line = NULL;

	/*
	 * echo "123
	 * 456
	 * 7
	 * " | grep 4
	 */
	const char *str = "echo \"123\n456\n7\n\" | grep 4";
	uint32_t len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_STDOUT, "out type");
	unit_check(!line->is_background, "not background");
	struct expr *e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "123\n456\n7\n") == 0, "arg[0]");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_PIPE, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "grep") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "4") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	parser_delete(p);
	unit_test_finish();
}

static void
test_logical_operators(void)
{
	unit_test_start();
	struct parser *p = parser_new();
	struct command_line *line = NULL;

	const char *str = "false && echo 123";
	uint32_t len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_STDOUT, "out type");
	unit_check(!line->is_background, "not background");
	struct expr *e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "false") == 0, "exe");
	unit_check(e->cmd.arg_count == 0, "arg count");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_AND, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "123") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	unit_msg("Multiple operators");
	str = "true || false || true && echo 123";
	len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_STDOUT, "out type");
	unit_check(!line->is_background, "not background");
	e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "true") == 0, "exe");
	unit_check(e->cmd.arg_count == 0, "arg count");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_OR, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "false") == 0, "exe");
	unit_check(e->cmd.arg_count == 0, "arg count");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_OR, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "true") == 0, "exe");
	unit_check(e->cmd.arg_count == 0, "arg count");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_AND, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "123") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	unit_msg("Logical operators and pipes");
	str = "echo 100 | grep 1 && echo 200 | grep 2";
	len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_STDOUT, "out type");
	unit_check(!line->is_background, "not background");
	e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "100") == 0, "arg[0]");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_PIPE, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "grep") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "1") == 0, "arg[0]");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_AND, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "200") == 0, "arg[0]");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_PIPE, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "grep") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "2") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	parser_delete(p);
	unit_test_finish();
}

static void
test_background(void)
{
	unit_test_start();
	struct parser *p = parser_new();
	struct command_line *line = NULL;

	const char *str = "sleep 0.5 && echo 'back sleep is done' > test.txt &";
	uint32_t len = strlen(str);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &str[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse");
	unit_check(line->out_type == OUTPUT_TYPE_FILE_NEW, "out type");
	unit_check(strcmp(line->out_file, "test.txt") == 0, "out file");
	unit_check(line->is_background, "is background");
	struct expr *e = line->head;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "sleep") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "0.5") == 0, "arg[0]");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_AND, "expr type");

	e = e->next;
	unit_check(e->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(e->cmd.exe, "echo") == 0, "exe");
	unit_check(e->cmd.arg_count == 1, "arg count");
	unit_check(strcmp(e->cmd.args[0], "back sleep is done") == 0, "arg[0]");
	unit_check(e->next == NULL, "no more exprs");
	command_line_delete(line);

	parser_delete(p);
	unit_test_finish();
}

static void
test_error_one(struct parser *p, const char *expr, enum parser_error err)
{
	struct command_line *line = NULL;
	uint32_t len = strlen(expr);
	for (uint32_t i = 0; i < len; ++i) {
		parser_feed(p, &expr[i], 1);
		unit_fail_if(parser_pop_next(p, &line) != PARSER_ERR_NONE);
		unit_fail_if(line != NULL);
	}
	parser_feed(p, "\n", 1);
	unit_check(parser_pop_next(p, &line) == err, "parse error");
	unit_check(line == NULL, "no line");
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse none");
}

static void
test_errors(void)
{
	unit_test_start();
	struct parser *p = parser_new();
	struct command_line *line = NULL;

	test_error_one(p, " | exe", PARSER_ERR_PIPE_WITH_NO_LEFT_ARG);
	test_error_one(p, "exe && |", PARSER_ERR_PIPE_WITH_LEFT_ARG_NOT_A_COMMAND);
	test_error_one(p, " && exe", PARSER_ERR_AND_WITH_NO_LEFT_ARG);
	test_error_one(p, "exe && &&", PARSER_ERR_AND_WITH_LEFT_ARG_NOT_A_COMMAND);
	test_error_one(p, " || exe", PARSER_ERR_OR_WITH_NO_LEFT_ARG);
	test_error_one(p, " exe && ||", PARSER_ERR_OR_WITH_LEFT_ARG_NOT_A_COMMAND);
	test_error_one(p, "exe > &&", PARSER_ERR_OUTOUT_REDIRECT_BAD_ARG);
	test_error_one(p, "exe >> &&", PARSER_ERR_OUTOUT_REDIRECT_BAD_ARG);
	test_error_one(p, "exe > test.txt & arg", PARSER_ERR_TOO_LATE_ARGUMENTS);
	test_error_one(p, "exe |", PARSER_ERR_ENDS_NOT_WITH_A_COMMAND);
	test_error_one(p, "exe &&", PARSER_ERR_ENDS_NOT_WITH_A_COMMAND);
	test_error_one(p, "exe ||", PARSER_ERR_ENDS_NOT_WITH_A_COMMAND);

	parser_feed(p, "echo\n", 5);
	unit_check(parser_pop_next(p, &line) == PARSER_ERR_NONE, "parse ok");
	unit_check(line->head->type == EXPR_TYPE_COMMAND, "expr type");
	unit_check(strcmp(line->head->cmd.exe, "echo") == 0, "exe");
	command_line_delete(line);

	parser_delete(p);
	unit_test_finish();
}

int
main(void)
{
	test_one_word();
	test_incomplete();
	test_two_words();
	test_escape_in_string();
	test_output_redirect();
	test_escape_outside_of_string();
	test_pipe();
	test_comments();
	test_multiline_string();
	test_logical_operators();
	test_background();
	test_errors();
	return 0;
}
