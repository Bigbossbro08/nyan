// Copyright 2016-2016 the nyan authors, LGPLv3+. See copying.md for legal info.
#ifndef NYAN_NYAN_PTR_CONTAINER_H_
#define NYAN_NYAN_PTR_CONTAINER_H_

#include <memory>


namespace nyan {

/**
 * Container class to contain some data, either by
 * owning one or by pointing to an existing one.
 */
template<typename T>
class NyanPtrContainer {
public:
	NyanPtrContainer()
		:
		is_owning{false},
		data_owned{nullptr},
		data_ptr{nullptr} {}

	/**
	 * Type of this container.
	 */
	using this_type = NyanPtrContainer<T>;

	/**
	 * Create a non-owning container from a raw pointer.
	 */
	NyanPtrContainer(T *val)
		:
		is_owning{false},
		data_owned{nullptr},
		data_ptr{val} {}

	/**
	 * Create an owning container from some unique ptr.
	 */
	NyanPtrContainer(std::unique_ptr<T> &&val)
		:
		is_owning{true},
		data_owned{std::move(val)},
		data_ptr{nullptr} {}

	/**
	 * Replace this container by another container.
	 */
	NyanPtrContainer(NyanPtrContainer &&other)
		:
		is_owning{other.is_owning},
		data_owned{std::move(other.data_owned)},
		data_ptr{other.data_ptr} {}

	/**
	 * Move assignment from another container.
	 */
	NyanPtrContainer &operator =(NyanPtrContainer &&other) {
		this->is_owning = other.is_owning;
		this->data_owned = std::move(other.data_owned);
		this->data_ptr = other.data_ptr;
		return *this;
	}

	// no copies
	NyanPtrContainer(const NyanPtrContainer &other) = delete;
	NyanPtrContainer &operator =(const NyanPtrContainer &other) = delete;

	virtual ~NyanPtrContainer() = default;


	/**
	 * Set the value to an owned unique_ptr.
	 */
	void set(std::unique_ptr<T> &&val) {
		this->is_owning = true;
		this->data_owned = std::move(val);
		this->data_ptr = nullptr;
	}

	/**
	 * Set the value to a non-owning pointer.
	 */
	void set(T *val) {
		this->is_owning = false;
		this->data_owned = nullptr;
		this->data_ptr = val;
	}


	/**
	 * Return a pointer to the contained data.
	 */
	T *get() const noexcept {
		if (this->is_owning) {
			return this->data_owned.get();
		} else {
			return this->data_ptr;
		}
	}


	/**
	 * Provide a reference to the data.
	 */
	T &operator *() const noexcept {
		return *this->get();
	}


	/**
	 * Dereference the data for member access.
	 */
	T *operator ->() const noexcept {
		return this->get();
	}


	/**
	 * Return true if this container has any data stored.
	 */
	bool has_data() const {
		return (this->get() != nullptr);
	}


	/**
	 * Return true if this container is a pointer
	 * and does not own a value.
	 */
	bool is_ptr() const {
		return not this->is_owning;
	}


	/**
	 * Compare the contained values.
	 */
	bool operator ==(const this_type &other) {
		return this->get() == other.get();
	}


	/**
	 * Check if contained values are not equal.
	 */
	bool operator !=(const this_type &other) {
		return this->get() != other.get();
	}

protected:
	bool is_owning;
	std::unique_ptr<T> data_owned;
	T *data_ptr;
};


} // namespace nyan

#endif
