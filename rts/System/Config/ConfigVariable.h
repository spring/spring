/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONFIG_VALUE_H
#define CONFIG_VALUE_H

#include <boost/utility.hpp>
#include <map>
#include <sstream>
#include <string>

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

/**
 * @brief Untyped configuration variable meta data.
 *
 * That is, meta data of a type that does not depend on the declared type
 * of the config variable.
 */
class ConfigVariableMetaData : public boost::noncopyable
{
public:
	typedef TypedStringConvertibleOptionalValue<std::string> OptionalString;
	typedef TypedStringConvertibleOptionalValue<int> OptionalInt;

	virtual ~ConfigVariableMetaData() {}

	/// @brief Get the default value of this config variable.
	virtual const StringConvertibleOptionalValue& GetDefaultValue() const = 0;

	/// @brief Get the minimum value of this config variable.
	virtual const StringConvertibleOptionalValue& GetMinimumValue() const = 0;

	/// @brief Get the maximum value of this config variable.
	virtual const StringConvertibleOptionalValue& GetMaximumValue() const = 0;

	/// @brief Get the safemode value of this config variable.
	virtual const StringConvertibleOptionalValue& GetSafemodeValue() const = 0;

	/// @brief Clamp a value using the declared minimum and maximum value.
	virtual std::string Clamp(const std::string& value) const = 0;

	std::string GetKey() const { return key; }
	std::string GetType() const { return type; }

	const OptionalString& GetDeclarationFile() const { return declarationFile; }
	const OptionalInt& GetDeclarationLine() const { return declarationLine; }
	const OptionalString& GetDescription() const { return description; }

protected:
	const char* key;
	const char* type;
	OptionalString declarationFile;
	OptionalInt declarationLine;
	OptionalString description;

	template<typename F> friend class ConfigVariableBuilder;
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
	ConfigVariableTypedMetaData(const char* k, const char* t) { key = k; type = t; }

	const StringConvertibleOptionalValue& GetDefaultValue() const { return defaultValue; }
	const StringConvertibleOptionalValue& GetMinimumValue() const { return minimumValue; }
	const StringConvertibleOptionalValue& GetMaximumValue() const { return maximumValue; }
	const StringConvertibleOptionalValue& GetSafemodeValue() const { return safemodeValue; }

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
		TypedStringConvertibleOptionalValue<T> temp;
		temp = TypedStringConvertibleOptionalValue<T>::FromString(value);
		if (minimumValue.IsSet()) {
			temp = std::max(temp.Get(), minimumValue.Get());
		}
		if (maximumValue.IsSet()) {
			temp = std::min(temp.Get(), maximumValue.Get());
		}
		return temp.ToString();
	}

protected:
	TypedStringConvertibleOptionalValue<T> defaultValue;
	TypedStringConvertibleOptionalValue<T> minimumValue;
	TypedStringConvertibleOptionalValue<T> maximumValue;
	TypedStringConvertibleOptionalValue<T> safemodeValue;

	template<typename F> friend class ConfigVariableBuilder;
};

/**
 * @brief Fluent interface to declare meta data of config variables
 *
 * Uses method chaining so that a config variable can be declared like this:
 *
 * CONFIG(int, Example)
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

	MAKE_CHAIN_METHOD(declarationFile, const char*);
	MAKE_CHAIN_METHOD(declarationLine, int);
	MAKE_CHAIN_METHOD(description, std::string);
	MAKE_CHAIN_METHOD(defaultValue,  T);
	MAKE_CHAIN_METHOD(minimumValue,  T);
	MAKE_CHAIN_METHOD(maximumValue,  T);
	MAKE_CHAIN_METHOD(safemodeValue, T);

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
	static void OutputMetaDataMap();

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
	static ConfigVariableTypedMetaData<T> cfgdata##name(#name, #T); \
	static ConfigVariable cfg##name = ConfigVariableBuilder<T>(cfgdata##name) \
		.declarationFile(__FILE__).declarationLine(__LINE__)

#endif // CONFIG_VALUE_H
