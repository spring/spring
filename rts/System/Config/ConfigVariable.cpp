/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ConfigVariable.h"
#include "System/Log/ILog.h"
#include <iostream>

using std::cout;
using std::map;
using std::string;

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

const ConfigVariableMetaData* ConfigVariable::GetMetaData(const string& key)
{
	const MetaDataMap& vars = GetMetaDataMap();
	MetaDataMap::const_iterator pos = vars.find(key);

	if (pos != vars.end()) {
		return pos->second;
	}
	else {
		return NULL;
	}
}

#ifdef DEBUG
CONFIG(std::string, test)
	.defaultValue("x y z")
	.description("\"quoted\", escaped: \\, \b, \f, \n, \r, \t");
#endif

/**
 * @brief Escape special characters and wrap in double quotes.
 */
static string Quote(const string& value)
{
	string esc(value);
	string::size_type pos = 0;
	while ((pos = esc.find_first_of("\"\\\b\f\n\r\t", pos)) != string::npos) {
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
	return "\"" + esc + "\"";
}

/**
 * @brief Call Quote if type is not bool, float or int.
 */
static string Quote(const string& type, const string& value)
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
	cout << "{\n";

	const MetaDataMap& mdm = GetMetaDataMap();
	for (MetaDataMap::const_iterator it = mdm.begin(); it != mdm.end(); ++it) {
		if (it != mdm.begin()) {
			cout << ",\n";
		}
		cout << it->second;
	}

	cout << "\n}\n";
}
