/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STRING_HASH_H
#define STRING_HASH_H

unsigned HashString(const char* s, size_t n);
static inline unsigned HashString(const std::string& s) { return (HashString(s.c_str(), s.size())); }



constexpr unsigned int hashString(const char* str, unsigned int length = -1u, unsigned int hash = 5381u)
{
	return ((*str) != 0 && length > 0) ? hashString(str + 1, length - 1, hash + (hash << 5) + *str) : hash;
}

constexpr unsigned int hashStringLower(const char* str, unsigned int length = -1u, unsigned int hash = 5381u)
{
	return ((*str) != 0 && length > 0) ? hashStringLower(str + 1, length - 1, hash + (hash << 5) + (*str + ('a' - 'A') * (*str >= 'A' && *str <= 'Z'))) : hash;
}

template<unsigned int length, unsigned int step = (length >> 5) + 1, unsigned int idx = length, unsigned int stop = length % step>
struct compileTimeHasher {
	static constexpr unsigned int hash(const char* str, unsigned int prev_hash = length) {
		return compileTimeHasher<length, step, idx - step, stop>::hash(str, prev_hash ^ ((prev_hash << 5) + (prev_hash >> 2) + ((unsigned char)str[idx - 1])));
	}
};

// stopping condition
template<unsigned int length, unsigned int step, unsigned int idx>
struct compileTimeHasher<length, step, idx, idx> {
	static constexpr unsigned int hash(const char* str, unsigned int prev_hash = length) {
		return prev_hash;
	}
};

#define COMPILE_TIME_HASH(str) compileTimeHasher<sizeof(str) - 1>::hash(str)

#endif

