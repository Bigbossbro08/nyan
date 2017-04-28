// Copyright 2016-2016 the nyan authors, LGPLv3+. See copying.md for legal info.
#ifndef NYAN_NYAN_VALUE_SET_H_
#define NYAN_NYAN_VALUE_SET_H_

#include <unordered_set>

#include "ops.h"
#include "value_set_base.h"


namespace nyan {

/**
 * NyanValue to store a unordered set of things.
 */
class NyanSet
	: public NyanSetBase<std::unordered_set<NyanValueContainer>> {

	// fetch the constructors
	using NyanSetBase<
		std::unordered_set<NyanValueContainer>>::NyanSetBase;

public:
	NyanSet();
	NyanSet(std::vector<NyanValueContainer> &values);

	std::string str() const override;
	std::string repr() const override;

	std::unique_ptr<NyanValue> copy() const override;

	bool add(NyanValueContainer &&value) override;
	bool contains(NyanValue *value) override;
	bool remove(NyanValue *value) override;

	const std::unordered_set<nyan_op> &allowed_operations(nyan_basic_type value_type) const override;
	const nyan_basic_type &get_type() const override;

protected:
	void apply_value(const NyanValue *value, nyan_op operation) override;
};

} // namespace nyan

#endif
