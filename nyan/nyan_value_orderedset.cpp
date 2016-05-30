// Copyright 2016-2016 the nyan authors, LGPLv3+. See copying.md for legal info.

#include "nyan_value_orderedset.h"

#include "nyan_value_container.h"


namespace nyan {

NyanOrderedSet::NyanOrderedSet() {}


NyanOrderedSet::NyanOrderedSet(const std::vector<NyanToken> &tokens) {
	throw NyanInternalError{"TODO"};
}


std::unique_ptr<NyanValue> NyanOrderedSet::copy() const {
	throw NyanInternalError{"TODO"};
}


std::string NyanOrderedSet::str() const {

	throw NyanInternalError{"TODO"};

	std::ostringstream builder;
	builder << "<";

	size_t cnt = 0;

	for (auto &value : this->values) {
		if (cnt > 0) {
			builder << ", ";
		}
		// TODO: fix the iterator!
		builder << value->value->str();
		cnt += 1;
	}

	builder << ">";
	return builder.str();
}


std::string NyanOrderedSet::repr() const {
	return this->str();
}


size_t NyanOrderedSet::hash() const {
	throw NyanError{"NyanOrderedSet is not hashable."};
}


bool NyanOrderedSet::add(NyanValueContainer &&value) {
	return this->values.add(std::move(value));
}


bool NyanOrderedSet::contains(NyanValue *value) {
	return this->values.contains(value);
}


bool NyanOrderedSet::remove(NyanValue *value) {
	throw NyanInternalError{"TODO"};
}


void NyanOrderedSet::apply_value(const NyanValue *value, nyan_op operation) {
	throw NyanInternalError{"TODO"};
}


bool NyanOrderedSet::equals(const NyanValue &other) const {
	throw NyanInternalError{"TODO"};
}


const std::unordered_set<nyan_op> &NyanOrderedSet::allowed_operations(nyan_type value_type) const {

	// TODO: use value_type to check the op-value-pair validity

	const static std::unordered_set<nyan_op> ops{
		nyan_op::ASSIGN,
		nyan_op::ADD_ASSIGN,
		nyan_op::UNION_ASSIGN,
		nyan_op::SUBTRACT_ASSIGN,
		nyan_op::MULTIPLY_ASSIGN,
		nyan_op::INTERSECT_ASSIGN,
	};

	return ops;
}


} // namespace nyan
