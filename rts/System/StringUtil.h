/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <algorithm>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>

#include "System/MainDefines.h"

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


static inline const char* StrCaseStr(const char* str, const char* sub) {
	const char* pos = nullptr;

	char lcstr[32768];
	char lcsub[32768];

	if (str == nullptr)
		return nullptr;
	if (sub == nullptr)
		return nullptr;

	std::strncpy(lcstr, str, sizeof(lcstr) - 1);
	std::strncpy(lcsub, sub, sizeof(lcsub) - 1);
	std::transform(lcstr, lcstr + sizeof(lcstr), lcstr, (int (*)(int)) tolower);
	std::transform(lcsub, lcsub + sizeof(lcsub), lcsub, (int (*)(int)) tolower);

	if ((pos = std::strstr(lcstr, lcsub)) == nullptr)
		return nullptr;

	return (str + (pos - lcstr));
}


static inline void StringToLower(const char* in, char* out, size_t len) {
	std::transform(in, in + len, out, (int (*)(int)) tolower);
}

static inline void StringToLowerInPlace(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), (int (*)(int)) tolower);
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
	if (str[0] == '"' && str[str.length() - 1] == '"')
		return str.substr(1, str.length() - 2);

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


static inline std::string IntToString(int i, const char* format = "%i")
{
	char buf[64];
	SNPRINTF(buf, sizeof(buf), format, i);
	return buf;
}

static inline std::string FloatToString(float f, const char* format = "%f")
{
	char buf[64];
	SNPRINTF(buf, sizeof(buf), format, f);
	return buf;
}

template<typename int_type = int>
static inline int_type StringToInt(std::string str, bool* failed = nullptr)
{
	StringTrimInPlace(str);
	std::istringstream stream(str);
	int_type buffer = 0;
	stream >> buffer;

	if (failed != nullptr)
		*failed = stream.fail();

	return buffer;
}

/**
 * Returns true of the argument string matches "0|n|no|f|false".
 * The matching is done case insensitive.
 */
bool StringToBool(std::string str);

bool StringStartsWith(const std::string& str, const char* prefix);
bool StringEndsWith(const std::string& str, const char* postfix);

/// Returns true if str starts with prefix
static inline bool StringStartsWith(const std::string& str, const std::string& prefix)
{
	return StringStartsWith(str, prefix.c_str());
}

/// Returns true if str ends with postfix
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
void InverseOrSetBool(bool& b, const std::string& argValue, const bool inverseArg = false);


namespace utf8 {
	char32_t GetNextChar(const std::string& text, int& pos);
	std::string FromUnicode(char32_t ch);


	static inline int CharLen(const std::string& str, int pos)
	{
		const auto oldPos = pos;
		utf8::GetNextChar(str, pos);
		return pos - oldPos;
	}

	static inline int NextChar(const std::string& str, int pos)
	{
		utf8::GetNextChar(str, pos);
		return pos;
	}

	static inline int PrevChar(const std::string& str, int pos)
	{
		int startPos = std::max(pos - 4, 0);
		int oldPos   = startPos;
		while (startPos < pos) {
			oldPos = startPos;
			utf8::GetNextChar(str, startPos);
		}
		return oldPos;
	}
};

#if !defined(UNITSYNC) && !defined(UNIT_TEST) && !defined(BUILDING_AI)
namespace zlib {
	std::vector<std::uint8_t> deflate(const std::uint8_t* inflData, unsigned long inflSize);
	std::vector<std::uint8_t> inflate(const std::uint8_t* deflData, unsigned long deflSize);

	std::vector<std::uint8_t> deflate(const std::vector<std::uint8_t>& inflData);
	std::vector<std::uint8_t> inflate(const std::vector<std::uint8_t>& deflData);
};
#endif //UNITSYNC

#endif // UTIL_H
