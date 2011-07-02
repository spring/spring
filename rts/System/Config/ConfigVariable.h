/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONFIG_VALUE_H
#define CONFIG_VALUE_H

#include <assert.h>
#include <boost/utility.hpp>
#include <map>
#include <sstream>
#include <string>

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
 * @brief Untyped configuration variable meta data.
 *
 * That is, meta data of a type that does not depend on the declared type
 * of the config variable.
 */
class ConfigVariableMetaData : public boost::noncopyable
{
public:
	virtual ~ConfigVariableMetaData() {}

	/// @brief Get the default value of this config variable as a string.
	virtual std::string GetDefaultValue() const = 0;

	/// @brief Returns true if and only if a default value has been set.
	virtual bool HasDefaultValue() const = 0;

	/// @brief Clamp a value using the declared minimum and maximum value.
	virtual std::string Clamp(const std::string& value) const = 0;

public:
	std::string key;
	const char* declarationFile;
	int declarationLine;
	Optional<std::string> description;
};

/**
 * @brief Typed configuration variable meta data.
 *
 * That is, meta data of the same type as the declared type
 * of the config variable.
 */
template<typename T>
class ConfigVariableTypedMetaData : public ConfigVariableMetaData
{
public:
	std::string GetDefaultValue() const
	{
		return ToString(defaultValue.Get());
	}

	bool HasDefaultValue() const
	{
		return defaultValue.IsSet();
	}

	/**
	 * @brief Clamp a value using the declared minimum and maximum value.
	 *
	 * Even though the input and the output is passed as string, the clamping
	 * happens in the domain of the declared type of the config variable.
	 * This may be a different type than what is read from the ConfigHandler.
	 *
	 * For example: a config variable declared as int will be clamped in the
	 * int domain even if ConfigHandler::GetString is used to fetch it.
	 *
	 * This guarantees the value will always be in range, even if Lua, a client
	 * of unitsync or erroneous Spring code uses the wrong getter.
	 */
	std::string Clamp(const std::string& value) const
	{
		T temp = FromString(value);
		if (minimumValue.IsSet()) {
			temp = std::max(temp, minimumValue.Get());
		}
		if (maximumValue.IsSet()) {
			temp = std::min(temp, maximumValue.Get());
		}
		return ToString(temp);
	}

public:
	Optional<T> defaultValue;
	Optional<T> minimumValue;
	Optional<T> maximumValue;

private:
	static std::string ToString(T value)
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
};

/**
 * @brief Fluent interface to declare meta data of config variables
 *
 * Uses method chaining so that a config variable can be declared like this:
 *
 * CONFIG(Example)
 *   .defaultValue(6)
 *   .minimumValue(1)
 *   .maximumValue(10)
 *   .description("This is an example");
 */
template<typename T>
class ConfigVariableBuilder : public boost::noncopyable
{
public:
	ConfigVariableBuilder(ConfigVariableTypedMetaData<T>& data) : data(&data) {}
	const ConfigVariableMetaData* GetData() const { return data; }

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
	ConfigVariableTypedMetaData<T>* data;
};

/**
 * @brief Configuration variable declaration
 *
 * The only purpose of this class is to store the meta data created by a
 * ConfigVariableBuilder in a global map of meta data as it is assigned to
 * an instance of this class.
 */
class ConfigVariable
{
public:
	typedef std::map<std::string, const ConfigVariableMetaData*> MetaDataMap;

	static const MetaDataMap& GetMetaDataMap();
	static const ConfigVariableMetaData* GetMetaData(const std::string& key);

private:
	static MetaDataMap& GetMutableMetaDataMap();
	static void AddMetaData(const ConfigVariableMetaData* data);

public:
	/// @brief Implicit conversion from ConfigVariableBuilder<T>.
	template<typename T>
	ConfigVariable(const ConfigVariableBuilder<T>& builder)
	{
		AddMetaData(builder.GetData());
	}
};

/**
 * @brief Macro to start the method chain used to declare a config variable.
 * @see ConfigVariableBuilder
 */
#define CONFIG(T, name) \
	static ConfigVariableTypedMetaData<T> cfgdata##name; \
	static ConfigVariable cfg##name = ConfigVariableBuilder<T>(cfgdata##name) \
		.key(#name).declarationFile(__FILE__).declarationLine(__LINE__)

#endif // CONFIG_VALUE_H
