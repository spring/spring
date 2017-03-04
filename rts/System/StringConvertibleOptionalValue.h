/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STRING_CONVERTIBLE_OPTIONAL_VALUE_H
#define STRING_CONVERTIBLE_OPTIONAL_VALUE_H

#include <string>
#include <sstream>
#include "System/Misc/NonCopyable.h"

/**
 * @brief Untyped base class for TypedStringConvertibleOptionalValue.
 */
class StringConvertibleOptionalValue : public spring::noncopyable
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

#endif // STRING_CONVERTIBLE_OPTIONAL_VALUE_H
