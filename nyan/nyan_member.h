// Copyright 2016-2016 the nyan authors, LGPLv3+. See copying.md for legal info.
#ifndef NYAN_NYAN_MEMBER_H_
#define NYAN_NYAN_MEMBER_H_

#include <memory>
#include <string>
#include <type_traits>
#include <unordered_set>

#include "nyan_location.h"
#include "nyan_ops.h"
#include "nyan_type.h"
#include "nyan_type_container.h"
#include "nyan_value.h"
#include "nyan_value_container.h"

namespace nyan {

class NyanObject;

/**
 * Stores a member of a NyanObject.
 * Also responsible for validating applied operators.
 */
class NyanMember {
public:
	/**
	 * Member without value.
	 */
	NyanMember(const NyanLocation &location, NyanTypeContainer &&type);

	/**
	 * Member with value.
	 */
	NyanMember(const NyanLocation &location,
	           NyanTypeContainer &&type, nyan_op operation,
	           NyanValueContainer &&value);

	NyanMember(NyanMember &&other);
	const NyanMember &operator =(NyanMember &&other);

	NyanMember(const NyanMember &other) = delete;
	const NyanMember &operator =(const NyanMember &other) = delete;

	virtual ~NyanMember() = default;

	/**
	 * String representation of this member.
	 */
	std::string str() const;

	/**
	 * Return the value of this member.
	 * @returns nullptr if there is no value yet.
	 */
	NyanValue *get_value_ptr() const;

	/**
	 * Get a member value and cast the result to the
	 * specified output type.
	 */
	template <typename T>
	T *get_value() const {
		static_assert(std::is_base_of<NyanValue, T>::value,
		              "only nyan value types are supported");

		return dynamic_cast<T *>(this->get_value_ptr());
	}

	/**
	 * Replace the value of this member.
	 */
	void set_value(NyanValueContainer &&val);

	/**
	 * Replace the value of this member within the container.
	 */
	void set_value(std::unique_ptr<NyanValue> &&val);

	/**
	 * Return the type of this member.
	 */
	NyanType *get_type() const;

	/**
	 * Provide the operation stored in the member.
	 */
	nyan_op get_operation() const;

	/**
	 * Save a previous result of `get_value` to bypass calculation
	 * next time.
	 */
	void cache_save(std::unique_ptr<NyanValue> &&value);

	/**
	 * Return the content of the value calculation cache.
	 * nullptr if the cache is empty.
	 */
	NyanValue *cache_get() const;

	/**
	 * Clear the value calculation cache.
	 */
	void cache_reset();

	/**
	 * Get the location where this member was defined.
	 */
	const NyanLocation &get_location() const;

protected:
	/**
	 * The type of this member.
	 * Either, this member defines the type, or it points
	 * to the definition at another member.
	 */
	NyanTypeContainer type;

	/**
	 * operation specified for this member.
	 */
	nyan_op operation;

	/**
	 * Value to cache the calculation result.
	 * It stores the result of the application of all operations on
	 * the inheritance tree.
	 */
	std::unique_ptr<NyanValue> cached_value;

	/**
	 * Value of just this member.
	 */
	NyanValueContainer value;

	/**
	 * Location where this member was defined.
	 */
	NyanLocation location;
};


} // namespace nyan

#endif
