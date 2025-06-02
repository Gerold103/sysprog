#pragma once

#include <list>
#include <optional>
#include <string>
#include <vector>

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
	~command();

	std::string exe;
	std::vector<char *> args;
};

enum expr_type {
	EXPR_TYPE_COMMAND,
	EXPR_TYPE_PIPE,
	EXPR_TYPE_AND,
	EXPR_TYPE_OR,
};

struct expr {
	expr_type type;
	// Valid if the type is COMMAND.
	command cmd;
};

enum output_type {
	OUTPUT_TYPE_STDOUT,
	OUTPUT_TYPE_FILE_NEW,
	OUTPUT_TYPE_FILE_APPEND,
};

struct command_line {
	std::list<expr> exprs;
	output_type out_type;
	// Valid if the out type is FILE.
	std::optional<std::string> out_file;
	bool is_background;
};

class parser {
public:
	void
	feed(const char *str, uint32_t len);

	parser_error
	pop_next(command_line *&out);

private:
	std::string m_buffer;
};
