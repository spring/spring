/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STRING_HASH_H
#define STRING_HASH_H

[[nodiscard]] uint32_t HashString(const char* s, size_t n);
[[nodiscard]] static inline uint32_t HashString(const std::string& s) { return (HashString(s.c_str(), s.size())); }



[[nodiscard]] constexpr uint32_t hashString(const char* str, uint32_t length = -1u, uint32_t hash = 5381u)
{
	return ((*str) != 0 && length > 0) ? hashString(str + 1, length - 1, hash + (hash << 5) + *str) : hash;
}

[[nodiscard]] constexpr uint32_t hashStringLower(const char* str, uint32_t length = -1u, uint32_t hash = 5381u)
{
	return ((*str) != 0 && length > 0) ? hashStringLower(str + 1, length - 1, hash + (hash << 5) + (*str + ('a' - 'A') * (*str >= 'A' && *str <= 'Z'))) : hash;
}

[[nodiscard]] constexpr uint32_t operator"" _hs(const char* str, std::size_t) noexcept {
	return hashString(str);
}


template<uint32_t length, uint32_t step = (length >> 5) + 1, uint32_t idx = length, uint32_t stop = length % step>
struct compileTimeHasher {
	[[nodiscard]] static constexpr uint32_t hash(const char* str, uint32_t prev_hash = length) {
		return compileTimeHasher<length, step, idx - step, stop>::hash(str, prev_hash ^ ((prev_hash << 5) + (prev_hash >> 2) + ((uint8_t)str[idx - 1])));
	}
};

// stopping condition
template<uint32_t length, uint32_t step, uint32_t idx>
struct compileTimeHasher<length, step, idx, idx> {
	[[nodiscard]] static constexpr uint32_t hash(const char* str, uint32_t prev_hash = length) {
		return prev_hash;
	}
};

#define COMPILE_TIME_HASH(str) compileTimeHasher<sizeof(str) - 1>::hash(str)

#endif

