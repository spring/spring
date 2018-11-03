/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ConfigVariable.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include <iostream>

/**
 * @brief Log an error about a ConfigVariableMetaData
 */
#define LOG_VAR(data, fmt, ...) \
	LOG_L(L_ERROR, "%s:%d: " fmt, data->GetDeclarationFile().Get().c_str(), data->GetDeclarationLine().Get(), ## __VA_ARGS__) \


ConfigVariable::MetaDataMap& ConfigVariable::GetMutableMetaDataMap()
{
	static MetaDataMap vars;
	return vars;
}

const ConfigVariable::MetaDataMap& ConfigVariable::GetMetaDataMap()
{
	return GetMutableMetaDataMap();
}

void ConfigVariable::AddMetaData(const ConfigVariableMetaData* data)
{
	MetaDataMap& vars = GetMutableMetaDataMap();
	MetaDataMap::const_iterator pos = vars.find(data->GetKey());

	if (pos != vars.end()) {
		LOG_VAR(data, "Duplicate config variable declaration \"%s\"", data->GetKey().c_str());
		LOG_VAR(pos->second, "  Previously declared here");
	}
	else {
		vars[data->GetKey()] = data;
	}
}

const ConfigVariableMetaData* ConfigVariable::GetMetaData(const std::string& key)
{
	const MetaDataMap& vars = GetMetaDataMap();
	MetaDataMap::const_iterator pos = vars.find(key);

	if (pos != vars.end()) {
		return pos->second;
	}
	else {
		return nullptr;
	}
}

#ifdef DEBUG
CONFIG(std::string, test)
	.defaultValue("x y z")
	.description("\"quoted\", escaped: \\, \b, \f, \n, \r, \t");
#endif


/**
 * @brief Call Quote if type is not bool, float or int.
 */
static inline std::string Quote(const std::string& type, const std::string& value)
{
	if (type == "bool" || type == "float" || type == "int") {
		return value;
	}
	else {
		return Quote(value);
	}
}

/**
 * @brief Write a ConfigVariableMetaData to a stream.
 */
static std::ostream& operator<< (std::ostream& out, const ConfigVariableMetaData* d)
{
	const char* const OUTER_INDENT = "  ";
	const char* const INDENT = "    ";

	out << OUTER_INDENT << Quote(d->GetKey()) << ": {\n";

#define KV(key, value) out << INDENT << Quote(#key) << ": " << (value) << ",\n"

	if (d->GetDeclarationFile().IsSet()) {
		KV(declarationFile, Quote(d->GetDeclarationFile().Get()));
	}
	if (d->GetDeclarationLine().IsSet()) {
		KV(declarationLine, d->GetDeclarationLine().Get());
	}
	if (d->GetDescription().IsSet()) {
		KV(description, Quote(d->GetDescription().Get()));
	}
	if (d->GetReadOnly().IsSet()) {
		KV(readOnly, d->GetReadOnly().Get());
	}
	if (d->GetDefaultValue().IsSet()) {
		KV(defaultValue, Quote(d->GetType(), d->GetDefaultValue().ToString()));
	}
	if (d->GetMinimumValue().IsSet()) {
		KV(minimumValue, Quote(d->GetType(), d->GetMinimumValue().ToString()));
	}
	if (d->GetMaximumValue().IsSet()) {
		KV(maximumValue, Quote(d->GetType(), d->GetMaximumValue().ToString()));
	}
	if (d->GetSafemodeValue().IsSet()) {
		KV(safemodeValue, Quote(d->GetType(), d->GetSafemodeValue().ToString()));
	}
	if (d->GetHeadlessValue().IsSet()) {
		KV(headlessValue, Quote(d->GetType(), d->GetHeadlessValue().ToString()));
	}
	if (d->GetDedicatedValue().IsSet()) {
		KV(dedicatedValue, Quote(d->GetType(), d->GetDedicatedValue().ToString()));
	}
	// Type is required.
	// Easiest to do this last because of the trailing comma that isn't there.
	out << INDENT << Quote("type") << ": " << Quote(d->GetType()) << "\n";

#undef KV

	out << OUTER_INDENT << "}";

	return out;
}

/**
 * @brief Output config variable meta data as JSON to stdout.
 *
 * This can be tested using, for example:
 *
 *	./spring --list-config-vars |
 *		python -c 'import json, sys; json.dump(json.load(sys.stdin), sys.stdout)'
 */
void ConfigVariable::OutputMetaDataMap()
{
	std::cout << "{\n";

	const MetaDataMap& mdm = GetMetaDataMap();
	for (MetaDataMap::const_iterator it = mdm.begin(); it != mdm.end(); ++it) {
		if (it != mdm.begin()) {
			std::cout << ",\n";
		}
		std::cout << it->second;
	}

	std::cout << "\n}\n";
}
