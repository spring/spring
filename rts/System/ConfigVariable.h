/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONFIG_VALUE_H
#define CONFIG_VALUE_H

namespace cfg
{
	class _NoDefault {};
	const _NoDefault NoDefault = _NoDefault();
}

/**
 * @brief Configuration value declaration
 */
class ConfigVariable
{
public:
	template<typename T>
	ConfigVariable(const std::string& key, const T& value, const std::string& comment, const char* declFile, int declLine) :
		next(head), key(key), defaultValue(ToString(value)), comment(comment), hasDefaultValue(true), declarationFile(declFile), declarationLine(declLine)
	{
		head = this;
	}

private:
	template<typename T>
	static std::string ToString(const T& value)
	{
		std::ostringstream buffer;
		buffer << value;
		return buffer.str();
	}

public:
	static ConfigVariable* head;

public:
	const ConfigVariable* next;
	const std::string key;
	const std::string defaultValue;
	const std::string comment;
	const bool hasDefaultValue;

	const char* const declarationFile;
	const int declarationLine;
};

// note: explicit specialization has to be at the namespace scope
template<> inline ConfigVariable::ConfigVariable<cfg::_NoDefault>(const std::string& key, const cfg::_NoDefault& value, const std::string& comment, const char* declFile, int declLine) :
	next(head), key(key), comment(comment), hasDefaultValue(false), declarationFile(declFile), declarationLine(declLine)
{
	head = this;
}

#define CONFIG(name, defVal) \
	static ConfigVariable cfg##name(#name, defVal, "", __FILE__, __LINE__)

#endif // CONFIG_VALUE_H
