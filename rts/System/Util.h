/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#include "System/maindefines.h"

/*
 * Pre-processor trickery, useful to create unique identifiers.
 * see http://stackoverflow.com/questions/461062/c-anonymous-variables
 */
#define _UTIL_CONCAT_SUB(start, end) \
	start##end
#define _UTIL_CONCAT(start, end) \
	_UTIL_CONCAT_SUB(start, end)



#define DO_ONCE(func) \
	struct do_once##func { do_once##func() {func();} }; static do_once##func do_once_var##func;

#define DO_ONCE_FNC(code) \
	struct _UTIL_CONCAT(doOnce, __LINE__) { _UTIL_CONCAT(doOnce, __LINE__)() { code; } }; static _UTIL_CONCAT(doOnce, __LINE__) _UTIL_CONCAT(doOnceVar, __LINE__);


static inline void StringToLowerInPlace(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))tolower);
}

static inline std::string StringToLower(std::string s)
{
	StringToLowerInPlace(s);
	return s;
}

/**
 * @brief Escape special characters and wrap in double quotes.
 */
static inline std::string Quote(std::string esc)
{
	std::string::size_type pos = 0;
	while ((pos = esc.find_first_of("\"\\\b\f\n\r\t", pos)) != std::string::npos) {
		switch (esc[pos]) {
			case '\"':
			case '\\': esc.insert(pos, "\\"); break;
			case '\b': esc.replace(pos, 1, "\\b"); break;
			case '\f': esc.replace(pos, 1, "\\f"); break;
			case '\n': esc.replace(pos, 1, "\\n"); break;
			case '\r': esc.replace(pos, 1, "\\r"); break;
			case '\t': esc.replace(pos, 1, "\\t"); break;
		}
		pos += 2;
	}

	std::ostringstream buf;
	buf << "\"" << esc << "\"";
	return buf.str();
}


/**
 * @brief Escape special characters and wrap in double quotes.
 */
static inline std::string UnQuote(const std::string& str)
{
	if (str[0] == '"' && str[str.length() - 1] == '"') {
		return str.substr(1, str.length() - 2);
	}
	return str;
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

/// strips any occurrences of characters in <chars> from <str>
std::string StringStrip(const std::string& str, const std::string& chars);

/// Removes leading and trailing whitespace from a string, in place.
void StringTrimInPlace(std::string& str, const std::string& ws = " \t\n\r");
/// Removes leading and trailing whitespace from a string, in a copy.
std::string StringTrim(const std::string& str, const std::string& ws = " \t\n\r");


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

template<typename int_type = int>
static inline int_type StringToInt(std::string str, bool* failed = NULL)
{
	StringTrimInPlace(str);
	std::istringstream stream(str);
	int_type buffer = 0;
	stream >> buffer;

	if (failed != NULL)
		*failed = stream.fail();

	return buffer;
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


/// Appends postfix, when it doesn't already ends with it
static inline void EnsureEndsWith(std::string* str, const char* postfix)
{
	if (!StringEndsWith(*str, postfix)) {
		*str += postfix;
	}
}


/**
 * Sets a bool according to the value encoded in a string.
 * The conversion works like this:
 * - ""  -> toggle-value
 * - "0" -> false
 * - "1" -> true
 */
void InverseOrSetBool(bool& container, const std::string& argValue, const bool inverseArg = false);


/// Helper function to avoid division by Zero
static inline float SafeDivide(const float a, const float b)
{
	if (b == 0.0f)
		return a;

	return (a / b);
}

namespace spring {
	template<typename T, typename TV>
	static auto find(T& c, const TV& v) -> decltype(c.end())
	{
		return std::find(c.begin(), c.end(), v);
	}

	template<typename T, typename UnaryPredicate>
	static void map_erase_if(T& c, UnaryPredicate p)
	{
		for(auto it = c.begin(); it != c.end(); ) {
			if( p(*it) ) it = c.erase(it);
			else ++it;
		}
	}
}


template<typename T, typename P>
static bool VectorEraseIf(std::vector<T>& v, const P& p)
{
	auto it = std::find_if(v.begin(), v.end(), p);

	if (it == v.end())
		return false;

	*it = v.back();
	v.pop_back();
	return true;
}

template<typename T>
static bool VectorErase(std::vector<T>& v, T e)
{
	auto it = std::find(v.begin(), v.end(), e);

	if (it == v.end())
		return false;

	*it = v.back();
	v.pop_back();
	return true;
}

template<typename T>
static bool VectorInsertUnique(std::vector<T>& v, T e, bool b = false)
{
	// do not assume uniqueness, test for it
	if (b && std::find(v.begin(), v.end(), e) != v.end())
		return false;

	// assume caller knows best, skip the test
	assert(b || std::find(v.begin(), v.end(), e) == v.end());
	v.push_back(e);
	return true;
}



/**
 * @brief Safe alternative to "delete obj;"
 * Safely deletes an object, by first setting the pointer to NULL and then
 * deleting.
 * This way, it is guaranteed that other objects can not access the object
 * through the pointer while the object is running its destructor.
 */
template<class T> void SafeDelete(T& a)
{
	T tmp = a;
	a = NULL;
	delete tmp;
}

/**
 * @brief Safe alternative to "delete [] obj;"
 * Safely deletes an array object, by first setting the pointer to NULL and then
 * deleting.
 * This way, it is guaranteed that other objects can not access the object
 * through the pointer while the object is running its destructor.
 */
template<class T> void SafeDeleteArray(T*& a)
{
	T* tmp = a;
	a = NULL;
	delete [] tmp;
}



char32_t Utf8GetNextChar(const std::string& text, int& pos);
std::string UnicodeToUtf8(char32_t ch);


static inline int Utf8CharLen(const std::string& str, int pos)
{
	const auto oldPos = pos;
	Utf8GetNextChar(str, pos);
	return pos - oldPos;
}

static inline int Utf8NextChar(const std::string& str, int pos)
{
	Utf8GetNextChar(str, pos);
	return pos;
}

static inline int Utf8PrevChar(const std::string& str, int pos)
{
	int startPos = std::max(pos - 4, 0);
	int oldPos   = startPos;
	while (startPos < pos) {
		oldPos = startPos;
		Utf8GetNextChar(str, startPos);
	}
	return oldPos;
}
#endif // UTIL_H
