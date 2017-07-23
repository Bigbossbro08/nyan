// Copyright 2016-2017 the nyan authors, LGPLv3+. See copying.md for legal info.
#pragma once


#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "error.h"

/*
 * Branch prediction tuning.
 * The expression is expected to be true (=likely) or false (=unlikely).
 */
#define likely(x)    __builtin_expect(!!(x), 1)
#define unlikely(x)  __builtin_expect(!!(x), 0)

#define BREAKPOINT asm("int $3")


namespace nyan {
namespace util {

/**
 * C++17 provides std::as_const, which does exactly the same:
 * Constify a value by prepending const.
 */
template<typename T> constexpr const T &as_const(T &t) noexcept {
	return t;
}

/**
 * Determine the size of a file.
 */
size_t file_size(const std::string &filename);

/**
 * Read a file from the filesystem and return the contents.
 * Optionally, open it in binary mode, which will leave newlines untouched.
 */
std::string read_file(const std::string &filename, bool binary=false);


/**
 * Demangles a symbol name.
 *
 * On failure, the mangled symbol name is returned.
 */
std::string demangle(const char *symbol);

/**
 * Return the demangled symbol name for a given code address.
 */
std::string symbol_name(const void *addr, bool require_exact_addr=true, bool no_pure_addrs=false);


/**
 * Just like python's "delim".join(container)
 * func is a function to extract the string
 * from each element of the container.
 * Fak u C++.
 */
template <typename T>
std::string strjoin(const std::string &delim,
                    const T &container,
                    const std::function<std::string(const typename T::value_type &)> func=[](const typename T::value_type &in) -> std::string {return in;}) {

	std::ostringstream builder;

	size_t cnt = 0;
	for (auto &entry : container) {
		if (cnt > 0) {
			builder << delim;
		}

		builder << func(entry);
		cnt += 1;
	}

	return builder.str();
}


/**
 * Split a string at a delimiter, push the result back in an iterator.
 * Why doesn't the fucking standard library have std::string::split(delimiter)?
 */
template<typename ret_t>
void split(const std::string &txt, char delimiter, ret_t result) {
	std::stringstream splitter;
	splitter.str(txt);
	std::string part;

	while (std::getline(splitter, part, delimiter)) {
		*result = part;
		result++;
	}
}


/**
 * Split a string at a delimiter into a vector.
 * Internally, uses the above iterator splitter.
 */
std::vector<std::string> split(const std::string &txt, char delim);


/**
 * Check if the given string ends with the ending.
 */
bool ends_with(const std::string &txt, const std::string &end);


/**
 * Extend a vector with elements, without destroying source one.
 */
template <typename T>
void vector_extend(std::vector<T> &vec, const std::vector<T> &ext) {
	vec.reserve(vec.size() + ext.size());
	vec.insert(std::end(vec), std::begin(ext), std::end(ext));
}


/*
 * Extend a vector with elements with move semantics.
 */
template <typename T>
void vector_extend(std::vector<T> &vec, std::vector<T> &&ext) {
	if (vec.empty()) {
		vec = std::move(ext);
	}
	else {
		vec.reserve(vec.size() + ext.size());
		std::move(std::begin(ext), std::end(ext), std::back_inserter(vec));
		ext.clear();
	}
}


/**
 * Creates a hash value as a combination of two other hashes. Can be called incrementally to create
 * hash value from several variables.
 */
size_t hash_combine(size_t hash1, size_t hash2);

} // namespace util
} // namespace nyan
