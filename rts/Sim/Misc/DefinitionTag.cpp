/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DefinitionTag.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include <iostream>
#ifndef _MSC_VER
#include <cxxabi.h>
#endif

using std::cout;


/**
 * @brief Log an error about a DefTagMetaData
 */
#define LOG_VAR(data, fmt, ...) \
	LOG_L(L_ERROR, "%s:%d: " fmt, data->GetDeclarationFile().Get().c_str(), data->GetDeclarationLine().Get(), ## __VA_ARGS__) \


DefType::~DefType() {
	for (const DefTagMetaData* md: tags)
		delete md;
}

std::vector<const DefType*>& DefType::GetTypes()
{
	static std::vector<const DefType*> tagtypes;
	return tagtypes;
}


DefType::DefType(const std::string& n) : name(n), luaTable(NULL) {
	GetTypes().push_back(this);
}


void DefType::AddMetaData(const DefTagMetaData* data)
{
	const auto key = data->GetInternalName();
	for (const DefTagMetaData* md: tags) {
		if (key == md->GetInternalName()) {
			LOG_VAR(data, "Duplicate config variable declaration \"%s\"", key.c_str());
			LOG_VAR(md,   "  Previously declared here");
			assert(false);
			return;
		}
	}

	tags.push_back(data);
}


const DefTagMetaData* DefType::GetMetaDataByInternalKey(const string& key)
{
	for (const DefTagMetaData* md: tags) {
		if (key == md->GetInternalName()) {
			return md;
		}
	}
	return nullptr;
}


const DefTagMetaData* DefType::GetMetaDataByExternalKey(const string& key)
{
	const std::string lkey = StringToLower(key);
	for (const DefTagMetaData* md: tags) {
		if (md->GetExternalName().IsSet()) {
			if (lkey == StringToLower(md->GetExternalName().Get()))
				return md;
		} else {
			if (lkey == StringToLower(md->GetInternalName()))
				return md;
		}

		if (lkey == StringToLower(md->GetFallbackName().Get()))
			return md;
	}
	return nullptr;
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

	bool first = true;
	for (auto metaData: tags) {
		if (!first) {
			cout << ",\n";
		}
		cout << metaData;
		first = false;
	}

	cout << "\n  }";
}

void DefType::OutputTagMap()
{
	cout << "{\n";

	bool first = true;
	for (const DefType* defType: GetTypes()) {
		if (!first) {
			cout << ",\n";
		}
		cout << "  " << Quote(defType->GetName()) << ": ";
		defType->OutputMetaDataMap();
		first = false;
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


void DefType::ReportUnknownTags(const std::string& instanceName, const LuaTable& luaTable, const std::string pre)
{
	std::vector<std::string> keys;
	luaTable.GetKeys(keys);
	for (const std::string& tag: keys) {
		const DefTagMetaData* meta = GetMetaDataByExternalKey(pre + tag);
		if (meta != nullptr)
			continue;

		if (luaTable.GetType(tag) == LuaTable::TABLE) {
			ReportUnknownTags(instanceName, luaTable.SubTable(tag), pre + tag + ".");
			continue;
		}

		LOG_L(L_WARNING, "%s: Unknown tag \"%s%s\" in \"%s\"", name.c_str(), pre.c_str(), tag.c_str(), instanceName.c_str());
	}
}


void DefType::Load(void* instance, const LuaTable& luaTable)
{
	this->luaTable = &luaTable;

	for (const DefInitializer& init: inits) {
		init(instance);
	}

	this->luaTable = nullptr;
}
