// Copyright 2016-2016 the nyan authors, LGPLv3+. See copying.md for legal info.

#include <nyan_error.h>

#include <sstream>

namespace nyan {

NyanError::NyanError(const std::string &msg)
	:
	msg{msg} {}

std::string NyanError::str() const {
	return this->msg;
}

ParserError::ParserError(const std::string &msg,
                         int line, int line_offset)
	:
	NyanError{msg},
	line{line},
	line_offset{line_offset} {}

std::string ParserError::str() const {
	std::ostringstream builder;

	if (this->line > 0) {
		builder << this->line << ":"
		        << this->line_offset << ": ";
	}

	builder << "\x1b[31;1merror:\x1b[m " << this->msg;
	return builder.str();
}


} // namespace nyan
