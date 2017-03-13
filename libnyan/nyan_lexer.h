// Copyright 2016-2016 the nyan authors, LGPLv3+. See copying.md for legal info.
#ifndef NYAN_LEXER_H_
#define NYAN_LEXER_H_

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>

// don't include the flex generated header if we come from the cpp file.
// srsly fak u flex.
#ifndef NYAN_LEXER_FUCK_YOU_FLEX
#  include "nyan_flex_lexer.h"
#endif

#include "nyan_error.h"
#include "nyan_token.h"


// number of spaces per indent
#define SPACES_PER_INDENT 4
#define SPACES_PER_INDENT_STR "4"

namespace nyan {

class NyanFile;

class NyanLexer : public NyanFlexLexer {
public:
	/**
	 * Create a lexer for the given file.
	 */
	NyanLexer(const NyanFile &file);
	virtual ~NyanLexer() = default;

	// no moves and copies
	NyanLexer(NyanLexer &&other) = delete;
	NyanLexer(const NyanLexer &other) = delete;
	NyanLexer &operator =(NyanLexer &&other) = delete;
	NyanLexer &operator =(const NyanLexer &other) = delete;

	/**
	 * Return the next available token.
	 */
	NyanToken get_next_token();

protected:
	/**
	 * Defined by the flex lexer generator.
	 * Produces a token by reading the input.
	 * Place the token in the queue.
	 */
	void generate_token();

	/**
	 * Measure the indentation of a line,
	 * Generate indentation tokens.
	 * Return the difference to the previous indent.
	 */
	int handle_indent(const char *line);

	/**
	 * Create a token with correct text position and value.
	 * Add the token to the queue.
	 */
	void token(token_type type);

	/**
	 * Tokenize error was encountered.
	 */
	TokenizeError error(const std::string &msg);

	/**
	 * Advance the line position by the length of the current token.
	 */
	void advance_linepos();

	/**
	 * Reset the line position to the beginning.
	 */
	void reset_linepos();

	/**
	 * The default line positon at the very beginning of one line.
	 */
	static constexpr int linepos_start = 0;

	/**
	 * The indentation stack remembers the levels of indent.
	 */
	std::vector<int> indent_stack;

	/**
	 * Filename used for tokenization.
	 */
	const NyanFile &file;

	/**
	 * String stream which is fed into the lexer.
	 */
	std::istringstream input;

	/**
	 * Current position in a line.
	 */
	int linepos;

	/**
	 * Available tokens.
	 */
	std::queue<NyanToken> tokens;

	/**
	 * Indicates whether the tokenization has reached the end.
	 */
	bool finished;
};


/**
 * Exception for lexer problems.
 */
class LexerError : public NyanFileError {
public:
	LexerError(const NyanLocation &location, const std::string &msg);
};


} // namespace nyan

#endif
