// Copyright 2016-2016 the nyan authors, LGPLv3+. See copying.md for legal info.
#ifndef NYAN_NYAN_VALUE_TEXT_H_
#define NYAN_NYAN_VALUE_TEXT_H_

#include <string>

#include "nyan_value.h"


namespace nyan {

/**
 * Nyan value to store text.
 */
class NyanText : public NyanValue {
public:
	NyanText(const std::string &value);
	NyanText(const NyanToken &token);

	std::unique_ptr<NyanValue> copy() const override;
	std::string str() const override;
	size_t hash() const override;

protected:
	void apply_value(const NyanValue *value, nyan_op operation) override;
	bool equals(const NyanValue &other) const override;
	const std::unordered_set<nyan_op> &allowed_operations(nyan_type value_type) const override;

	std::string value;
};

} // namespace nyan

#endif
