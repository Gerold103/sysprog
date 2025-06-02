#include "parser.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

namespace
{
enum token_type {
	TOKEN_TYPE_NONE,
	TOKEN_TYPE_STR,
	TOKEN_TYPE_NEW_LINE,
	TOKEN_TYPE_PIPE,
	TOKEN_TYPE_AND,
	TOKEN_TYPE_OR,
	TOKEN_TYPE_OUT_NEW,
	TOKEN_TYPE_OUT_APPEND,
	TOKEN_TYPE_BACKGROUND,
};

struct token {
	char *
	strdup() const
	{
		size_t len = data.size();
		char *res = new char[len + 1];
		memcpy(res, data.data(), len);
		res[len] = 0;
		return res;
	}

	token_type type = TOKEN_TYPE_NONE;
	std::string data;
};
}

command::~command()
{
	for (char *arg : args)
		delete[] arg;
}

void
parser::feed(const char *str, uint32_t len)
{
	m_buffer.append(str, len);
}

static uint32_t
parse_token(const char *pos, const char *end, token &out)
{
	out.type = TOKEN_TYPE_NONE;
	out.data.resize(0);

	const char *begin = pos;
	while (pos < end) {
		if (!isspace(*pos))
			break;
		if (*pos == '\n') {
			out.type = TOKEN_TYPE_NEW_LINE;
			return pos + 1 - begin;
		}
		++pos;
	}
	char quote = 0;
	while (pos < end) {
		char c = *pos;
		switch(c) {
		case '\'':
		case '"':
			if (quote == 0) {
				quote = c;
				++pos;
				if (pos == end)
					return 0;
				continue;
			}
			if (quote != c)
				goto append_and_next;
			out.type = TOKEN_TYPE_STR;
			return pos + 1 - begin;
		case '\\':
			if (quote == '\'')
				goto append_and_next;
			if (quote == '"') {
				++pos;
				if (pos == end)
					return 0;
				c = *pos;
				switch (c)
				{
				case '\\':
					goto append_and_next;
				case '\n':
					++pos;
					continue;
				case '"':
					goto append_and_next;
				default:
					break;
				}
				out.data.push_back('\\');
				goto append_and_next;
			}
			assert(quote == 0);
			++pos;
			if (pos == end)
				return 0;
			c = *pos;
			if (c == '\n') {
				++pos;
				continue;
			}
			goto append_and_next;
		case '&':
		case '|':
		case '>':
			if (quote != 0)
				goto append_and_next;
			if (!out.data.empty()) {
				out.type = TOKEN_TYPE_STR;
				return pos - begin;
			}
			++pos;
			if (pos == end)
				return 0;
			if (*pos == c) {
				switch(c) {
				case '&':
					out.type = TOKEN_TYPE_AND;
					break;
				case '|':
					out.type = TOKEN_TYPE_OR;
					break;
				case '>':
					out.type = TOKEN_TYPE_OUT_APPEND;
					break;
				default:
					assert(false);
					break;
				}
				++pos;
			} else {
				switch(c) {
				case '&':
					out.type = TOKEN_TYPE_BACKGROUND;
					break;
				case '|':
					out.type = TOKEN_TYPE_PIPE;
					break;
				case '>':
					out.type = TOKEN_TYPE_OUT_NEW;
					break;
				default:
					assert(false);
					break;
				}
			}
			return pos - begin;
		case ' ':
		case '\t':
		case '\r':
			if (quote != 0)
				goto append_and_next;
			assert(!out.data.empty());
			out.type = TOKEN_TYPE_STR;
			return pos + 1 - begin;
		case '\n':
			if (quote != 0)
				goto append_and_next;
			assert(!out.data.empty());
			out.type = TOKEN_TYPE_STR;
			return pos - begin;
		case '#':
			if (quote != 0)
				goto append_and_next;
			if (!out.data.empty()) {
				out.type = TOKEN_TYPE_STR;
				return pos - begin;
			}
			++pos;
			while (pos < end) {
				if (*pos == '\n') {
					out.type = TOKEN_TYPE_NEW_LINE;
					return pos + 1 - begin;
				}
				++pos;
			}
			return 0;
		default:
			goto append_and_next;
		}
	append_and_next:
		out.data.push_back(c);
		++pos;
	}
	return 0;
}

parser_error
parser::pop_next(command_line *&out)
{
	command_line *line = new command_line();
	char *pos = m_buffer.data();
	const char *begin = pos;
	char *end = pos + m_buffer.size();
	token tok;
	parser_error res = PARSER_ERR_NONE;

	while (pos < end) {
		uint32_t used = parse_token(pos, end, tok);
		if (used == 0)
			goto return_no_line;
		pos += used;
		switch(tok.type) {
		case TOKEN_TYPE_STR: {
			if (!line->exprs.empty() &&
			    line->exprs.back().type == EXPR_TYPE_COMMAND) {
				line->exprs.back().cmd.args.push_back(tok.strdup());
				continue;
			}
			expr e;
			e.type = EXPR_TYPE_COMMAND;
			e.cmd.exe = std::move(tok.data);
			line->exprs.push_back(std::move(e));
			continue;
		}
		case TOKEN_TYPE_NEW_LINE: {
			// Skip new lines.
			if (line->exprs.empty())
				continue;
			goto close_and_return;
		}
		case TOKEN_TYPE_PIPE: {
			if (line->exprs.empty()) {
				res = PARSER_ERR_PIPE_WITH_NO_LEFT_ARG;
				goto return_error;
			}
			if (line->exprs.back().type != EXPR_TYPE_COMMAND) {
				res = PARSER_ERR_PIPE_WITH_LEFT_ARG_NOT_A_COMMAND;
				goto return_error;
			}
			expr e;
			e.type = EXPR_TYPE_PIPE;
			line->exprs.push_back(std::move(e));
			continue;
		}
		case TOKEN_TYPE_AND: {
			if (line->exprs.empty()) {
				res = PARSER_ERR_AND_WITH_NO_LEFT_ARG;
				goto return_error;
			}
			if (line->exprs.back().type != EXPR_TYPE_COMMAND) {
				res = PARSER_ERR_AND_WITH_LEFT_ARG_NOT_A_COMMAND;
				goto return_error;
			}
			expr e;
			e.type = EXPR_TYPE_AND;
			line->exprs.push_back(std::move(e));
			continue;
		}
		case TOKEN_TYPE_OR: {
			if (line->exprs.empty()) {
				res = PARSER_ERR_OR_WITH_NO_LEFT_ARG;
				goto return_error;
			}
			if (line->exprs.back().type != EXPR_TYPE_COMMAND) {
				res = PARSER_ERR_OR_WITH_LEFT_ARG_NOT_A_COMMAND;
				goto return_error;
			}
			expr e;
			e.type = EXPR_TYPE_OR;
			line->exprs.push_back(std::move(e));
			continue;
		}
		case TOKEN_TYPE_OUT_NEW:
		case TOKEN_TYPE_OUT_APPEND:
		case TOKEN_TYPE_BACKGROUND:
			goto close_and_return;
		default:
			assert(false);
		}
	}
	goto return_no_line;

close_and_return:
	if (tok.type == TOKEN_TYPE_OUT_NEW || tok.type == TOKEN_TYPE_OUT_APPEND)
	{
		if (tok.type == TOKEN_TYPE_OUT_NEW)
			line->out_type = OUTPUT_TYPE_FILE_NEW;
		else
			line->out_type = OUTPUT_TYPE_FILE_APPEND;
		uint32_t used = parse_token(pos, end, tok);
		if (used == 0)
			goto return_no_line;
		pos += used;
		if (tok.type != TOKEN_TYPE_STR) {
			res = PARSER_ERR_OUTOUT_REDIRECT_BAD_ARG;
			goto return_error;
		}
		line->out_file = std::move(tok.data);
		used = parse_token(pos, end, tok);
		if (used == 0)
			goto return_no_line;
		pos += used;
	}
	if (tok.type == TOKEN_TYPE_BACKGROUND) {
		line->is_background = true;
		uint32_t used = parse_token(pos, end, tok);
		if (used == 0)
			goto return_no_line;
		pos += used;
	}
	if (tok.type == TOKEN_TYPE_NEW_LINE) {
		assert(!line->exprs.empty());
		m_buffer.erase(0, pos - begin);
		if (line->exprs.back().type != EXPR_TYPE_COMMAND) {
			res = PARSER_ERR_ENDS_NOT_WITH_A_COMMAND;
			goto return_no_line;
		}
		res = PARSER_ERR_NONE;
		out = line;
		return res;
	}
	res = PARSER_ERR_TOO_LATE_ARGUMENTS;
	goto return_error;

return_error:
	// Try to skip the whole current line. It can't be executed but can't just crash
	// here because of that.
	while (pos < end) {
		uint32_t used = parse_token(pos, end, tok);
		if (used == 0)
			break;
		pos += used;
		if (tok.type == TOKEN_TYPE_NEW_LINE) {
			m_buffer.erase(0, pos - begin);
			goto return_no_line;
		}
	}
	res = PARSER_ERR_NONE;
	goto return_no_line;

return_no_line:
	delete line;
	out = nullptr;
	return res;
}
