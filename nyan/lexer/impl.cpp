// Copyright 2017-2017 the nyan authors, LGPLv3+. See copying.md for legal info.

#include "impl.h"

#include "../file.h"

namespace nyan::lexer {

Impl::Impl(const std::shared_ptr<File> &file)
	:
	yyFlexLexer{},
	file{file},
	input{file->get_content()} {

	// set the input stream in the flex base class
	this->switch_streams(&this->input, nullptr);

	// The base indentation is zero of course.
	this->indent_stack.push_back(0);
}

/*
 * Generate tokens until the queue has on available to return.
 * Return tokens from the queue until it's empty.
 */
Token Impl::generate_token() {
	if (this->tokens.empty()) {
		this->yylex();
	}

	if (not this->tokens.empty()) {
		auto ret = this->tokens.front();
		this->tokens.pop();
		return ret;
	}

	// if generate_token did not generate a token:
	throw this->error("internal error.");
}


/*
 * Fetch the current lexer state and throw an error.
 */
TokenizeError Impl::error(const std::string &msg) {
	return TokenizeError{
		Location{
			this->file,
			this->yylineno,
			this->linepos - this->yyleng,
			this->yyleng
		},
		msg
	};
}

void Impl::endline() {
	// ENDLINE is not an acceptable first token.
	// Optimize for consecutive ENDLINE tokens: keep only one.
	if (not tokens.empty() and tokens.back().type != token_type::ENDLINE) {
		/* don't assign the `\n` for the next line */
		this->yylineno--;
		this->token(token_type::ENDLINE);
		this->yylineno++;
	}
	// Reset the line position to the beginning.
	this->linepos = linepos_start;
}

/*
 * Fetch the current lexer state variables and create a token.
 */
void Impl::token(token_type type) {

	int token_start = this->linepos - this->yyleng;

	// to register open and close parenthesis
	// for correct line-wrap-indentation.
	this->track_brackets(type, token_start);

	if (token_needs_payload(type)) {
		this->tokens.push(Token{
			this->file,
			this->yylineno,
			token_start,
			this->yyleng,
			type,
			this->yytext
		});
	}
	else {
		this->tokens.push(Token{
			this->file,
			this->yylineno,
			token_start,
			this->yyleng,
			type
		});
	}
}

/*
 * Remember where the current open bracket is
 * so that the indentation can check if the depth is correct.
 */
void Impl::track_brackets(token_type type, int token_start) {

	// opening brackets
	if (type == token_type::LPAREN or
	    type == token_type::LANGLE or
	    type == token_type::LBRACKET or
	    type == token_type::LBRACE) {

		// Track bracket type and indentation.
		// The position after the ( is exactly the expected indent
		// for hanging brackets.
		this->bracket_stack.push_back(
			Bracket{
				type,
				token_start + 1
			}
		);

		this->possibly_hanging = true;
		return;
	}
	// closing brackets
	else if (type == token_type::RPAREN or
	         type == token_type::RANGLE or
	         type == token_type::RBRACKET or
	         type == token_type::RBRACE) {

		if (this->bracket_stack.empty()) {
			throw this->error("unexpected closing bracket, "
			                  "as no opening one is known");
		}

		Bracket &matching_open_bracket = this->bracket_stack.back();

		// test if bracket actually matches
		if (not matching_open_bracket.matches(type)) {
			std::ostringstream builder;
			builder << "non-matching bracket: expected '"
			        << matching_open_bracket.matching_type_str()
			        << "' but got '" << token_type_str(type) << "'";
			throw this->error(builder.str());
		}

		if (not matching_open_bracket.closing_indent_ok(token_start)) {
			std::ostringstream builder;
			builder << "wrong indentation of bracket: expected "
			        << matching_open_bracket.get_closing_indent()
			        << " indentation spaces (it is currently at "
			        << token_start << " spaces)";
			throw this->error(builder.str());
		}

		this->bracketcloseindent_expected = false;
		this->bracket_stack.pop_back();
	}
	// newline directly after opening bracket
	// means regular indentation has to follow
	// and the bracket pair doesn't hang.
	else if (not this->bracket_stack.empty() and
	         this->possibly_hanging and
	         type == token_type::ENDLINE) {

		// the bracket is followed by a newline directly,
		// thus is not hanging.
		this->bracket_stack.back().doesnt_hang(
			this->indent_stack.back()
		);
	}
	else if (not this->bracket_stack.empty() and
	         this->bracketcloseindent_expected) {
		std::ostringstream builder;
		builder << ("expected closing bracket or content "
		            "at indentation with ")
		        << this->bracket_stack.back().get_content_indent()
		        << " spaces (you start at " << token_start << " spaces)";
		throw this->error(builder.str());
	}

	this->possibly_hanging = false;
}

/*
 * measure the indentation of a line
 */
void Impl::handle_indent() {
	const char *line = this->yytext;
	// measure current indent
	int depth = 0;
	while (*line == ' ') {
		depth++;
		line++;
	}

	// Indentation depth of the last line
	int last_depth = 0;

	// regular indent is enforced when not in a bracket pair
	if (this->bracket_stack.empty()) {
		if ((depth % SPACES_PER_INDENT) > 0) {
			std::ostringstream builder;
			builder << "indentation requires exactly "
					<< SPACES_PER_INDENT
					<< " spaces per level";
			throw this->error(builder.str());
		}
	}
	// we're in a pair of brackets,
	// there the indentation is way funnier.
	else {

		// check if the content indentation is correct.
		int expected = this->bracket_stack.back().get_content_indent();
		if (depth != expected) {
			// if the expected depth is not correct,
			// then the only thing that is allowed is
			// the closing bracket.
			// the check will be done for the next token in
			// `track_brackets`.
			this->bracketcloseindent_expected = true;
		}

		// don't need to track the indent stack,
		// this is done in the bracket tracking now.
		return;
	}

	if (not this->indent_stack.empty()) {
		int previous_depth = last_depth = this->indent_stack.back();

		if (depth == previous_depth) {
			// same indent level
		}
		else if (depth < previous_depth) {
			// current line is further left than the previous one

			// pop indent stack until current level is reached
			while (previous_depth > depth) {
				int delta = previous_depth - depth;
				while (delta > 0) {
					delta -= SPACES_PER_INDENT;
					this->token(token_type::DEDENT);
				}

				this->indent_stack.pop_back();
				previous_depth = this->indent_stack.back();
			}
		}
		else {
			// new indent level
			this->indent_stack.push_back(depth);
			int delta = depth - last_depth;
			while (delta > 0) {
				delta -= SPACES_PER_INDENT;
				this->token(token_type::INDENT);
			}
		}
	}
	else {
		throw InternalError{"indentation stack ran empty!?!?"};
	}
}

} // namespace nyan::lexer
