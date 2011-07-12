/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "ConfigVariable.h"
#include "System/Log/ILog.h"

using std::map;
using std::string;

/**
 * @brief Log an error about a ConfigVariableMetaData
 */
#define LOG_VAR(data, fmt, ...) \
	LOG_L(L_ERROR, "%s:%d: " fmt, data->declarationFile, data->declarationLine, ## __VA_ARGS__)


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
	MetaDataMap::const_iterator pos = vars.find(data->key);

	if (pos != vars.end()) {
		LOG_VAR(data, "Duplicate config variable declaration \"%s\"\n", data->key.c_str());
		LOG_VAR(pos->second, "  Previously declared here\n");
	}
	else {
		vars[data->key] = data;
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
