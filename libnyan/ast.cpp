// Copyright 2016-2017 the nyan authors, LGPLv3+. See copying.md for legal info.

#include "ast.h"

#include <iostream>
#include <sstream>

#include "ops.h"

using namespace std::string_literals;

namespace nyan {


std::string ASTBase::str() const {
	std::ostringstream builder;
	this->strb(builder);
	return builder.str();
}


const std::vector<ASTObject> &AST::get_objects() const {
	return this->objects;
}


AST::AST(util::Iterator<Token> &tokens) {
	while (tokens.full()) {
		auto token = tokens.next();
		if (token->type == token_type::ID) {
			this->objects.push_back(ASTObject{*token, tokens});
		}
		else if (token->type == token_type::ENDFILE) {
			// we're done!
			if (tokens.empty()) {
				return;
			}
			else {
				throw Error{"some token came after EOF."};
			}
		}
		else {
			throw ASTError{"expected object name, but got", *token};
		}
	}
}

ASTObject::ASTObject(const Token &name,
                             util::Iterator<Token> &tokens)
	:
	name{name} {

	auto token = tokens.next();

	if (token->type == token_type::LANGLE) {
		this->ast_targets(tokens);
		token = tokens.next();
	}

	if (token->type == token_type::LBRACKET) {
		this->ast_inheritance_mod(tokens);
		token = tokens.next();
	}

	if (token->type == token_type::LPAREN) {
		this->ast_inheritance(tokens);
	} else {
		throw ASTError("create the object with (), i got", *token);
	}

	token = tokens.next();
	if (token->type != token_type::COLON) {
		throw ASTError("expected a : but instead encountered", *token);
	}

	token = tokens.next();
	if (token->type != token_type::ENDLINE) {
		throw ASTError("expected a newline there is", *token);
	}

	token = tokens.next();
	if (token->type != token_type::INDENT) {
		throw ASTError("expected indentation but instead there's", *token);
	}

	this->ast_members(tokens);
}

void ASTObject::ast_targets(util::Iterator<Token> &tokens) {
	auto token = tokens.next();
	if (token->type == token_type::ID) {
		this->target = *token;
	}
	else {
		throw ASTError("expected identifier, encountered", *token);
	}

	token = tokens.next();

	if (token->type != token_type::RANGLE) {
		throw ASTError("expected > as patch target end, there is", *token);
	}
}

void ASTObject::ast_inheritance_mod(util::Iterator<Token> &tokens) {
	bool expect_comma = false;
	auto token = tokens.next();

	while (token->type != token_type::RBRACKET) {
		if (token->type == token_type::COMMA) {
			expect_comma = false;
			token = tokens.next();
		}
		else if (expect_comma == true) {
			throw ASTError("expected comma, encountered", *token);
		}

		nyan_op action = op_from_token(*token);

		if (action != nyan_op::ADD) {
			throw ASTError("expected + operator,"
			               "instead got: '"s + token->get() +
			               "' when using", *token);
		}
		token = tokens.next();

		// add parent
		if (token->type == token_type::ID) {
			if (action == nyan_op::ADD) {
				this->inheritance_add.push_back(*token);
			}
			expect_comma = true;
		}
		else {
			throw ASTError("expected identifier, encountered", *token);
		}

		token = tokens.next();
	}
}

void ASTObject::ast_inheritance(util::Iterator<Token> &tokens) {
	this->inheritance = this->comma_list(tokens, token_type::RPAREN);
}

void ASTObject::ast_members(util::Iterator<Token> &tokens) {
	auto token = tokens.next();

	while (token->type != token_type::DEDENT and
	       token->type != token_type::ENDFILE) {

		if (token->type == token_type::ID) {
			this->members.push_back(ASTMember(*token, tokens));
		}
		else if (token->type == token_type::PASS or
		         token->type == token_type::ELLIPSIS) {
			// "empty" member entry.
			token = tokens.next();
			if (token->type != token_type::ENDLINE and
			    token->type != token_type::ENDFILE) {
				throw ASTError("expected newline after pass or '...', "
				               "but got", *token);
			}

		} else {
			throw ASTError("expected member identifier, but got", *token);
		}

		token = tokens.next();
	}
}


ASTMember::ASTMember(const Token &name,
                             util::Iterator<Token> &tokens)
	:
	name{name} {

	auto token = tokens.next();
	bool had_def_or_decl = false;

	// type specifier (the ": text" etc part)
	if (token->type == token_type::COLON) {
		token = tokens.next();

		if (token->type == token_type::ID) {
			this->type = ASTMemberType{*token, tokens};
			had_def_or_decl = true;
		} else {
			throw ASTError{"expected type name, instead got", *token};
		}

		token = tokens.next();
	}

	// value assigning
	if (token->type == token_type::OPERATOR) {
		this->operation = op_from_token(*token);

		if (this->operation == nyan_op::INVALID) {
			throw ASTError{"invalid operation", *token};
		}

		token = tokens.next();

		if (token->type == token_type::LANGLE or
		    token->type == token_type::LBRACE) {
			// multi-value container

			nyan_container_type ctype;
			if (token->type == token_type::LANGLE) {
				ctype = nyan_container_type::ORDEREDSET;
			} else if (token->type == token_type::LBRACE) {
				ctype = nyan_container_type::SET;
			} else {
				throw Error{"unhandled multi value container type"};
			}

			this->value = ASTMemberValue{ctype, tokens};
		}
		else {
			// single-value
			this->value = ASTMemberValue{*token};
		}

		had_def_or_decl = true;

		token = tokens.next();
	}
	else if (had_def_or_decl == false) {
		throw ASTError("expected type declaration ( : type ) "
		               "or value ( = something), instead got",
		               *token);
	}

	if (token->type != token_type::ENDLINE and
	    token->type != token_type::ENDFILE) {
		throw ASTError("expected newline after member entry, but got",
		               *token);
	}
}


ASTMemberType::ASTMemberType()
	:
	exists{false},
	has_payload{false} {}


ASTMemberType::ASTMemberType(const Token &name,
                                     util::Iterator<Token> &tokens)
	:
	exists{true},
	name{name},
	has_payload{false} {

	auto token = tokens.next();
	if (token->type == token_type::LPAREN) {
		token = tokens.next();
		if (token->type == token_type::ID) {
			this->payload = *token;
			this->has_payload = true;
		}
		else {
			throw ASTError("expected type identifier, but got", *token);
		}

		token = tokens.next();

		if (token->type != token_type::RPAREN) {
			throw ASTError("expected closing parens, but encountered", *token);
		}
	} else {
		tokens.reinsert(token);
	}
}


ASTMemberValue::ASTMemberValue()
	:
	exists{false} {}


ASTMemberValue::ASTMemberValue(const Token &token)
	:
	exists{true},
	container_type{nyan_container_type::SINGLE} {

	this->values.push_back(token);
}


ASTMemberValue::ASTMemberValue(nyan_container_type type,
                                       util::Iterator<Token> &tokens)
	:
	exists{true},
	container_type{type} {

	auto token = tokens.next();

	bool had_value = false;

	// add values until the matching parenthesis
	while (true) {
		if (this->container_type == nyan_container_type::SET and
		    token->type == token_type::RBRACE) {
			break;
		}
		else if (this->container_type == nyan_container_type::ORDEREDSET and
		         token->type == token_type::RANGLE) {
			break;
		}

		if (had_value) {
			if (token->type == token_type::COMMA) {
				token = tokens.next();
			}
			else {
				throw ASTError{"expected comma, but got", *token};
			}
		}

		this->values.push_back(*token);
		had_value = true;

		// now the container is over, or a comma must follow
		token = tokens.next();
	}
}


std::vector<Token> ASTBase::comma_list(
	util::Iterator<Token> &tokens, token_type end) const {

	std::vector<Token> ret;

	auto token = tokens.next();
	bool expect_comma = false;

	while (token->type != end) {
		if (token->type == token_type::COMMA) {
			expect_comma = false;
			token = tokens.next();
		}
		else if (expect_comma == true) {
			throw ASTError("expected comma, encountered", *token);
		}

		if (token->type == token_type::ID) {
			ret.push_back(*token);
			expect_comma = true;
		}
		else {
			throw ASTError("expected identifier, encountered", *token);
		}
		token = tokens.next();
	}
	return ret;
}


void AST::strb(std::ostringstream &builder) const {
	builder << "### nyan tree ###" << std::endl;

	size_t count = 0;
	for (auto &obj : this->objects) {
		builder << std::endl << "# [object " << count << "]" << std::endl;
		obj.strb(builder);
		count += 1;
	}
}


void ASTObject::strb(std::ostringstream &builder) const {
	builder << this->name.get();

	auto token_str = [](const auto &in) {
		return in.get();
	};

	// print <target>
	if (this->target.exists()) {
		builder << "<"
		        << this->target.get()
		        << ">";
	}


	if (this->inheritance_add.size() > 0) {
		builder << "[+"
		        << util::strjoin<Token>(", +", this->inheritance_add,
		                                    token_str)
		        << "]";
	}

	builder << "("
	        << util::strjoin<Token>(", ", this->inheritance, token_str)
	        << "):"
	        << std::endl;


	if (this->members.size() > 0) {
		for (auto &member : this->members) {
			builder << "    ";
			member.strb(builder);
		}
	}
	else {
		builder << "    pass" << std::endl;
	}
}


void ASTMember::strb(std::ostringstream &builder) const {
	builder << this->name.get();

	if (this->type.exists) {
		builder << " : ";
		this->type.strb(builder);
	}

	if (this->value.exists) {
		builder << " "
		        << op_to_string(this->operation)
		        << " ";

		this->value.strb(builder);
	}

	builder << std::endl;
}


void ASTMemberType::strb(std::ostringstream &builder) const {
	builder << this->name.get();

	if (this->has_payload) {
		builder << "(" << this->payload.get() << ")";
	}
}


void ASTMemberValue::strb(std::ostringstream &builder) const {
	switch (this->container_type) {
	case nyan_container_type::SINGLE:
		builder << this->values[0].get();
		return;

	case nyan_container_type::SET:
		builder << "{"; break;

	case nyan_container_type::ORDEREDSET:
		builder << "<"; break;

	default:
		throw Error{"unhandled container type"};
	}

	bool comma_active = false;
	for (auto &value : this->values) {
		if (comma_active) {
			builder << ", ";
		}
		builder << value.get();
		comma_active = true;
	}

	switch (this->container_type) {
	case nyan_container_type::SET:
		builder << "}"; break;

	case nyan_container_type::ORDEREDSET:
		builder << ">"; break;

	default:
		throw Error{"unhandled container type"};
	}
}


ASTError::ASTError(const std::string &msg, const Token &token,
                   bool add_token)
	:
	FileError{Location{token}, ""} {

	if (add_token) {
		std::ostringstream builder;
		builder << msg << ": "
		        << token_type_str(token.type);
		this->msg = builder.str();
	}
	else {
		this->msg = msg;
	}
}

} // namespace nyan
