#include <string>
#include "StringHash.h"

#ifndef CHAR_BIT
#define CHAR_BIT 8
#else
static_assert(CHAR_BIT == 8, "");
#endif

unsigned HashString(const char* s, size_t n)
{
	unsigned hash = 0;

	for (size_t i = 0; (i < n || n == std::string::npos); ++i) {
		if (s[i] == 0)
			break;

		hash += s[i];
		hash ^= (hash << 7) | (hash >> (sizeof(hash) * CHAR_BIT - 7));
	}

	return hash;
}

static_assert(hashStringLower("abcdABCDwxyz") == hashStringLower("ABCDabcdWXYZ"), "hashStringLower does not work");

