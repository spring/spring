#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "maindefines.h"



#ifdef __cplusplus

#include <string>
#include <algorithm>

static inline void StringToLowerInPlace(std::string &s)
{
	std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))tolower);
}

static inline std::string StringToLower(std::string s)
{
	StringToLowerInPlace(s);
	return s;
}

//! replace all characters matching 'c' in a string by 'd'
static inline std::string& StringReplaceInPlace(std::string& s, char c, char d)
{
	size_t i = 0;
	while (i < s.size()) {
		if (s[i] == c) {
			s[i] = d;
		}
		i++;
	}

	return s;
}

/**
 * Replaces all instances of <code>from</code> to <code>to</code>
 * in <code>text</code>.
 */
std::string StringReplace(const std::string& text,
                          const std::string& from,
                          const std::string& to);

static inline std::string GetFileExt(const std::string& s)
{
	size_t l = s.length();
#ifdef WIN32
	//! windows eats dots and spaces at the end of filenames
	while (l > 0) {
		if (s[l-1]=='.') {
			l--;
		} else if (s[l-1]==' ') {
			l--;
		} else break;
	}
#endif
	size_t i = s.rfind('.', l);
	if (i != std::string::npos) {
		return StringToLower(s.substr(i+1, l - i));
	}
	return "";
}

static inline std::string IntToString(int i, const std::string& format = "%i")
{
	char buf[64];
	SNPRINTF(buf, sizeof(buf), format.c_str(), i);
	return std::string(buf);
}

/**
 * @brief Safely delete object by first setting pointer to NULL and then deleting.
 * This way it is guaranteed other objects can not access the object through the
 * pointer while the object is running it's destructor.
 */
template<class T> void SafeDelete(T &a)
{
	T tmp = a;
	a = NULL;
	delete tmp;
}



namespace proc {
	#if defined(__GNUC__)
	__attribute__((__noinline__))
	void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d);
	#else
	void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d);
	#endif

	unsigned int GetProcMaxStandardLevel();
	unsigned int GetProcMaxExtendedLevel();
	unsigned int GetProcSSEBits();
}

#endif // __cplusplus

#endif // UTIL_H
