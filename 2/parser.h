#pragma once

#include <list>
#include <optional>
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <vector>

struct parser;

enum parser_error {
	PARSER_ERR_NONE,
	PARSER_ERR_PIPE_WITH_NO_LEFT_ARG,
	PARSER_ERR_PIPE_WITH_LEFT_ARG_NOT_A_COMMAND,
	PARSER_ERR_AND_WITH_NO_LEFT_ARG,
	PARSER_ERR_AND_WITH_LEFT_ARG_NOT_A_COMMAND,
	PARSER_ERR_OR_WITH_NO_LEFT_ARG,
	PARSER_ERR_OR_WITH_LEFT_ARG_NOT_A_COMMAND,
	PARSER_ERR_OUTOUT_REDIRECT_BAD_ARG,
	PARSER_ERR_TOO_LATE_ARGUMENTS,
	PARSER_ERR_ENDS_NOT_WITH_A_COMMAND,
};

struct command {
	std::string exe;
	std::vector<std::string> args;
};

enum expr_type {
	EXPR_TYPE_COMMAND,
	EXPR_TYPE_PIPE,
	EXPR_TYPE_AND,
	EXPR_TYPE_OR,
};

struct expr {
	enum expr_type type = EXPR_TYPE_COMMAND;
	/** Valid if the type is COMMAND. */
	std::optional<command> cmd;
};

enum output_type {
	OUTPUT_TYPE_STDOUT,
	OUTPUT_TYPE_FILE_NEW,
	OUTPUT_TYPE_FILE_APPEND,
};

struct command_line {
	std::list<expr> exprs;
	enum output_type out_type = OUTPUT_TYPE_STDOUT;
	/** Non-empty if the out type is FILE. */
	std::string out_file;
	bool is_background = false;
};

struct parser *
parser_new(void);

void
parser_feed(struct parser *p, const char *str, uint32_t len);

enum parser_error
parser_pop_next(struct parser *p, struct command_line **out);

void
parser_delete(struct parser *p);
