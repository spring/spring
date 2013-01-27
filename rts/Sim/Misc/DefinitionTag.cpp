/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DefinitionTag.h"
#include "System/Log/ILog.h"
#include <iostream>
#ifndef _MSC_VER
#include <cxxabi.h>
#endif

using std::cout;
using std::map;
using std::string;


/**
 * @brief Log an error about a DefTagMetaData
 */
#define LOG_VAR(data, fmt, ...) \
	LOG_L(L_ERROR, "%s:%d: " fmt, data->GetDeclarationFile().Get().c_str(), data->GetDeclarationLine().Get(), ## __VA_ARGS__) \


DefType::~DefType() {
	for (MetaDataMap::const_iterator it = map.begin(); it != map.end(); ++it)
		delete it->second;
}

std::map<std::string, const DefType*>& DefType::GetTypes()
{
	static std::map<std::string, const DefType*> tagtypes;
	return tagtypes;
}


DefType::DefType(const std::string& name) : luaTable(NULL) {
	GetTypes()[name] = this;
}


void DefType::AddMetaData(const DefTagMetaData* data)
{
	MetaDataMap::const_iterator pos = map.find(data->GetKey());

	if (pos != map.end()) {
		LOG_VAR(data, "Duplicate config variable declaration \"%s\"", data->GetKey().c_str());
		LOG_VAR(pos->second, "  Previously declared here");
	}
	else {
		map[data->GetKey()] = data;
	}
}

const DefTagMetaData* DefType::GetMetaData(const string& key)
{
	MetaDataMap::const_iterator pos = map.find(key);

	if (pos != map.end()) {
		return pos->second;
	}
	else {
		return NULL;
	}
}


std::string DefTagMetaData::GetTypeName(const std::type_info& typeInfo)
{
	// demangle typename
#ifndef _MSC_VER
	int status;
	char* ctname = abi::__cxa_demangle(typeInfo.name(), 0, 0, &status);
	const std::string tname = ctname;
	free(ctname);
#else
	const std::string tname(typeInfo.name()); // FIXME?
#endif
	return tname;
}


/**
 * @brief Call Quote if type is not bool, float or int.
 */
static inline string Quote(const string& type, const string& value)
{
	if (type == "std::string") {
		return Quote(value);
	}
	return value;
}


/**
 * @brief Write a DefTagMetaData to a stream.
 */
static std::ostream& operator<< (std::ostream& out, const DefTagMetaData* d)
{
	const char* const OUTER_INDENT = "    ";
	const char* const INDENT = "      ";

	const std::string tname = DefTagMetaData::GetTypeName(d->GetTypeInfo());

	out << OUTER_INDENT << Quote(d->GetKey()) << ": {\n";

#define KV(key, value) out << INDENT << Quote(#key) << ": " << (value) << ",\n"

	if (d->GetDeclarationFile().IsSet()) {
		KV(declarationFile, Quote(d->GetDeclarationFile().Get()));
	}
	if (d->GetDeclarationLine().IsSet()) {
		KV(declarationLine, d->GetDeclarationLine().Get());
	}
	if (d->GetExternalName().IsSet()) {
		KV(internalName, Quote(d->GetInternalName()));
	}
	if (d->GetFallbackName().IsSet()) {
		KV(fallbackName, Quote(d->GetFallbackName().Get()));
	}
	if (d->GetDescription().IsSet()) {
		KV(description, Quote(d->GetDescription().Get()));
	}
	if (d->GetDefaultValue().IsSet()) {
		KV(defaultValue, Quote(tname, d->GetDefaultValue().ToString()));
	}
	if (d->GetMinimumValue().IsSet()) {
		KV(minimumValue, Quote(tname, d->GetMinimumValue().ToString()));
	}
	if (d->GetMaximumValue().IsSet()) {
		KV(maximumValue, Quote(tname, d->GetMaximumValue().ToString()));
	}
	if (d->GetScaleValue().IsSet()) {
		KV(scaleValue, Quote(tname, d->GetScaleValue().ToString()));
	}
	if (d->GetScaleValueStr().IsSet()) {
		KV(scaleValueString, Quote(d->GetScaleValueStr().ToString()));
	}
	if (d->GetTagFunctionStr().IsSet()) {
		KV(tagFunction, Quote(d->GetTagFunctionStr().ToString()));
	}
	// Type is required.
	// Easiest to do this last because of the trailing comma that isn't there.
	out << INDENT << Quote("type") << ": " << Quote(tname) << "\n";

#undef KV

	out << OUTER_INDENT << "}";

	return out;
}

/**
 * @brief Output config variable meta data as JSON to stdout.
 *
 * This can be tested using, for example:
 *
 *	./spring --list-def-tags |
 *		python -c 'import json, sys; json.dump(json.load(sys.stdin), sys.stdout)'
 */
void DefType::OutputMetaDataMap() const
{
	cout << "{\n";

	for (MetaDataMap::const_iterator it = map.begin(); it != map.end(); ++it) {
		if (it != map.begin()) {
			cout << ",\n";
		}
		cout << it->second;
	}

	cout << "\n  }";
}

void DefType::OutputTagMap()
{
	cout << "{\n";

	std::map<std::string, const DefType*>& tagtypes = GetTypes();
	for (std::map<std::string, const DefType*>::const_iterator it = tagtypes.begin(); it != tagtypes.end(); ++it) {
		if (it != tagtypes.begin()) {
			cout << ",\n";
		}
		cout << "  " << Quote(it->first) << ": ";
		it->second->OutputMetaDataMap();
	}

	cout << "\n}\n";
}


void DefType::CheckType(const DefTagMetaData* meta, const std::type_info& want)
{
	assert(meta);
	if (meta->GetTypeInfo() != want)
		LOG_L(L_ERROR, "DEFTAG \"%s\" defined with wrong typevalue \"%s\" should be \"%s\"", meta->GetKey().c_str(), DefTagMetaData::GetTypeName(meta->GetTypeInfo()).c_str(), DefTagMetaData::GetTypeName(want).c_str());
	assert(meta->GetTypeInfo() == want);
}
