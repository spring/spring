/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "DefaultConfigSource.h"
#include "ConfigVariable.h"
#include "System/LogOutput.h"


DefaultConfigSource::DefaultConfigSource()
{
	const ConfigVariable::MetaDataMap& vars = ConfigVariable::GetMetaDataMap();

	for (ConfigVariable::MetaDataMap::const_iterator it = vars.begin(); it != vars.end(); ++it) {
		const ConfigVariableData* metadata = it->second;
		if (metadata->HasDefaultValue()) {
			data[metadata->key] = metadata->GetDefaultValue();
		}
	}
}
