/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>
#include <algorithm>

#include "System/maindefines.h"

#define DO_ONCE(func) \
	struct do_once##func { do_once##func() {func();} }; static do_once##func do_once_var##func;

static inline void StringToLowerInPlace(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))tolower);
}

static inline std::string StringToLower(std::string s)
{
	StringToLowerInPlace(s);
	return s;
}

static inline std::string Quote(std::string s)
{
	std::ostringstream buf;
	buf << "\"" << s << "\"";
	return buf.str();
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

/// Removes leading and trailing whitespace from a string, in place.
void StringTrimInPlace(std::string& str);
/// Removes leading and trailing whitespace from a string, in a copy.
std::string StringTrim(const std::string& str);


static inline std::string IntToString(int i, const std::string& format = "%i")
{
	char buf[64];
	SNPRINTF(buf, sizeof(buf), format.c_str(), i);
	return std::string(buf);
}

static inline std::string FloatToString(float f, const std::string& format = "%f")
{
	char buf[64];
	SNPRINTF(buf, sizeof(buf), format.c_str(), f);
	return std::string(buf);
}

/**
 * Returns true of the argument string matches "0|n|no|f|false".
 * The matching is done case insensitive.
 */
bool StringToBool(std::string str);

/// Returns true if str starts with prefix
bool StringStartsWith(const std::string& str, const char* prefix);
static inline bool StringStartsWith(const std::string& str, const std::string& prefix)
{
	return StringStartsWith(str, prefix.c_str());
}

/// Returns true if str ends with postfix
bool StringEndsWith(const std::string& str, const char* postfix);
static inline bool StringEndsWith(const std::string& str, const std::string& postfix)
{
	return StringEndsWith(str, postfix.c_str());
}

/**
 * @brief Safely delete object by first setting pointer to NULL and then deleting.
 * This way it is guaranteed other objects can not access the object through the
 * pointer while the object is running it's destructor.
 */
template<class T> void SafeDelete(T& a)
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

// set.erase(iterator++) is prone to crash with MSVC
template <class S, class I>
inline I set_erase(S &s, I i) {
#ifdef _MSC_VER
		return s.erase(i);
#else
		s.erase(i++);
		return i;
#endif
}


#endif // UTIL_H
