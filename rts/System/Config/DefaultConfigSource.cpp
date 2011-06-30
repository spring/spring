/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "DefaultConfigSource.h"
#include "ConfigVariable.h"
#include "System/LogOutput.h"

using std::map;
using std::string;

#define LOG_VAR(var) \
	LogObject() << (var)->data->declarationFile << ":" << (var)->data->declarationLine << ": "

DefaultConfigSource::DefaultConfigSource()
{
	for (const ConfigVariable* var = ConfigVariable::head; var != NULL; var = var->next) {
		LOG_VAR(var) << "ConfigVariable var(\"" << var->data->key << "\", \"" << var->data->GetDefaultValue() << "\")\n";

		map<string, const ConfigVariable*>::const_iterator pos = vars.find(var->data->key);

		if (pos != vars.end()) {
			LOG_VAR(var) << "Duplicate config variable declaration \"" << var->data->key << "\"\n";
			LOG_VAR(pos->second) << "  Previously declared here\n";
		}
		else {
			vars[var->data->key] = var;
		}

		if (var->data->HasDefaultValue()) {
			data[var->data->key] = var->data->GetDefaultValue();
		}
	}
}
