/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONFIG_VALUE_H
#define CONFIG_VALUE_H

#include <assert.h>
#include <boost/utility.hpp>
#include <sstream>

/**
 * @brief Wraps a value and detects whether it has been assigned to.
 */
template<typename T>
class Optional : public boost::noncopyable
{
public:
	Optional() : isSet(false) {}
	void operator=(const T& x) { value = x; isSet = true; }
	const T& Get() const { return value; }
	bool IsSet() const { return isSet; }

private:
	T value;
	bool isSet;
};

template<typename T>
std::ostream& operator<<(std::ostream& stream, const Optional<T>& o)
{
	if (o.IsSet()) {
		stream << o.Get();
	}
	return stream;
}

/**
 * @brief Data members for config variable that don't depend on type.
 */
class ConfigVariableData : public boost::noncopyable
{
public:
	virtual ~ConfigVariableData() {}
	virtual std::string GetDefaultValue() const = 0;
	virtual bool HasDefaultValue() const = 0;

public:
	std::string key;
	const char* declarationFile;
	int declarationLine;
	Optional<std::string> description;
};

/**
 * @brief Data members for config variable that do depend on type.
 */
template<typename T>
class ConfigVariableTypedData : public ConfigVariableData
{
public:
	std::string GetDefaultValue() const
	{
		std::ostringstream buf;
		buf << defaultValue;
		return buf.str();
	}

	bool HasDefaultValue() const
	{
		return defaultValue.IsSet();
	}

public:
	Optional<T> defaultValue;
	Optional<T> minimumValue;
	Optional<T> maximumValue;
};

/**
 * @brief Fluent interface to construct ConfigVariables
 * Uses method chaining.
 */
template<typename T>
class ConfigVariableBuilder : public boost::noncopyable
{
public:
	ConfigVariableBuilder(ConfigVariableTypedData<T>& data) : data(&data) {}
	const ConfigVariableData* GetData() const { return data; }

#define MAKE_CHAIN_METHOD(property, type) \
	ConfigVariableBuilder& property(type const& x) { \
		data->property = x; \
		return *this; \
	}

	MAKE_CHAIN_METHOD(key, std::string);
	MAKE_CHAIN_METHOD(declarationFile, const char*);
	MAKE_CHAIN_METHOD(declarationLine, int);
	MAKE_CHAIN_METHOD(description, std::string);
	MAKE_CHAIN_METHOD(defaultValue, T);
	MAKE_CHAIN_METHOD(minimumValue, T);
	MAKE_CHAIN_METHOD(maximumValue, T);

#undef MAKE_CHAIN_METHOD

private:
	ConfigVariableTypedData<T>* data;
};

/**
 * @brief Configuration variable meta data
 */
class ConfigVariable
{
public:
	/// @brief Implicit conversion from ConfigVariableBuilder<T>.
	template<typename T>
	ConfigVariable(const ConfigVariableBuilder<T>& builder) :
		next(NULL), data(builder.GetData())
	{
	}

	/// @brief Copy constructor
	ConfigVariable(const ConfigVariable& other) :
		next(head), data(other.data)
	{
		assert(other.next == NULL);
		head = this;
	}

public:
	static ConfigVariable* head;

public:
	const ConfigVariable* next;
	const ConfigVariableData* data;

private:
	/// @brief No assignment allowed
	ConfigVariable& operator=(const ConfigVariable&);
};

#define CONFIG(T, name) \
	static ConfigVariableTypedData<T> cfgdata##name; \
	static const ConfigVariable cfg##name = ConfigVariableBuilder<T>(cfgdata##name) \
		.key(#name).declarationFile(__FILE__).declarationLine(__LINE__)

#endif // CONFIG_VALUE_H
