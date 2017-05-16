// Copyright 2016-2017 the nyan authors, LGPLv3+. See copying.md for legal info.
#pragma once


#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include "error.h"
#include "location.h"
#include "ops.h"
#include "token.h"
#include "type.h"
#include "util.h"

namespace nyan {


/**
 * Base class for nyan AST classes.
 */
class ASTBase {
	friend class Parser;
public:
	virtual ~ASTBase() = default;

	/**
	 * Return a string representation of this AST element
	 * and maybe its children.
	 */
	std::string str() const;

	/**
	 * Add token values to the returned vector until the end token is
	 * encountered.
	 */
	std::vector<Token> comma_list(util::Iterator<Token> &tokens,
	                                  token_type end) const;

protected:
	virtual void strb(std::ostringstream &builder, int indentlevel=0) const = 0;
};


/**
 * AST representation of a member type declaration.
 */
class ASTMemberType : ASTBase {
	friend class ASTMember;
	friend class Parser;
	friend class Type;
public:
	ASTMemberType();
	ASTMemberType(const Token &name, util::Iterator<Token> &tokens);

	void strb(std::ostringstream &builder, int indentlevel=0) const override;

protected:
	bool exists;
	Token name;
	bool has_payload;
	Token payload;
};


/**
 * AST representation of a member value.
 */
class ASTMemberValue : public ASTBase {
	friend class Parser;
	friend class ASTMember;

public:
	ASTMemberValue();
	ASTMemberValue(nyan_container_type type,
	                   util::Iterator<Token> &tokens);
	ASTMemberValue(const Token &token);

	void strb(std::ostringstream &builder, int indentlevel=0) const override;

protected:
	bool exists;
	nyan_container_type container_type;

	std::vector<Token> values;
};


/**
 * The abstract syntax tree representation of a member entry.
 */
class ASTMember : public ASTBase {
	friend class Parser;
public:
	ASTMember(const Token &name, util::Iterator<Token> &tokens);

	void strb(std::ostringstream &builder, int indentlevel=0) const override;

protected:
	Token name;
	nyan_op operation;
	ASTMemberType type;
	ASTMemberValue value;
};


/**
 * An import in a nyan file is represented by this AST entry.
 */
class ASTImport : public ASTBase {
	friend class Parser;
public:
	ASTImport(const Token &name, util::Iterator<Token> &tokens);

	void strb(std::ostringstream &builder, int indentlevel=0) const override;

protected:
	Token namespace_name;
	Token alias;
};


/**
 * The abstract syntax tree representation of a nyan object.
 */
class ASTObject : public ASTBase {
	friend class Parser;
public:
	ASTObject(const Token &name, util::Iterator<Token> &tokens);

	void ast_targets(util::Iterator<Token> &tokens);
	void ast_inheritance_mod(util::Iterator<Token> &tokens);
	void ast_inheritance(util::Iterator<Token> &tokens);
	void ast_members(util::Iterator<Token> &tokens);

	void strb(std::ostringstream &builder, int indentlevel=0) const override;

protected:
	Token name;
	Token target;
	std::vector<Token> inheritance_add;
	std::vector<Token> inheritance;
	std::vector<ASTMember> members;
	std::vector<ASTObject> objects;
};


/**
 * Abstract syntax tree root.
 */
class AST : public ASTBase {
	friend class Parser;
public:
	AST(util::Iterator<Token> &tokens);

	void strb(std::ostringstream &builder, int indentlevel=0) const override;
	const std::vector<ASTObject> &get_objects() const;
	const std::vector<ASTImport> &get_imports() const;

protected:
	std::vector<ASTImport> imports;
	std::vector<ASTObject> objects;
};


/**
 * AST creation failure
 */
class ASTError : public FileError {
public:
	ASTError(const std::string &msg, const Token &token,
	         bool add_token=true);
};

} // namespace nyan
