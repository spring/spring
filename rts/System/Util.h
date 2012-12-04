/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>
#include <algorithm>
#include <boost/utility.hpp>

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

static inline int StringToInt(std::string str, bool* failed)
{
	StringTrimInPlace(str);
	std::istringstream stream(str);
	int buffer = 0;
	stream >> buffer;
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

/// Helper function to avoid division by Zero
static inline float SafeDivide(const float a, const float b)
{
	if (b==0)
		return a;
	else
		return a/b;
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




/**
 * @brief Untyped base class for TypedStringConvertibleOptionalValue.
 */
class StringConvertibleOptionalValue : public boost::noncopyable
{
public:
	StringConvertibleOptionalValue() : isSet(false) {}
	virtual ~StringConvertibleOptionalValue() {}
	virtual std::string ToString() const = 0;
	bool IsSet() const { return isSet; }

protected:
	bool isSet;
};

/**
 * @brief Wraps a value and detects whether it has been assigned to.
 */
template<typename T>
class TypedStringConvertibleOptionalValue : public StringConvertibleOptionalValue
{
public:
	TypedStringConvertibleOptionalValue<T>& operator=(const T& x) {
		value = x;
		isSet = true;
		return *this;
	}
	const T& Get() const { return value; }

	std::string ToString() const
	{
		std::ostringstream buf;
		buf << value;
		return buf.str();
	}

	static T FromString(const std::string& value)
	{
		std::istringstream buf(value);
		T temp;
		buf >> temp;
		return temp;
	}

protected:
	T value;
};

/**
 * @brief Specialization for std::string
 *
 * This exists because 1) converting from std::string to std::string is a no-op
 * and 2) converting from std::string to std::string using std::istringstream
 * will treat spaces as word boundaries, which we do not want.
 */
template<>
class TypedStringConvertibleOptionalValue<std::string> : public StringConvertibleOptionalValue
{
	typedef std::string T;

public:
	void operator=(const T& x) { value = x; isSet = true; }
	const T& Get() const { return value; }

	std::string ToString() const { return value; }
	static T FromString(const std::string& value) { return value; }

protected:
	T value;
};


#endif // UTIL_H
