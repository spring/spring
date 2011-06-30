/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "DefaultConfigSource.h"
#include "ConfigVariable.h"
#include "LogOutput.h"

using std::map;
using std::string;

#define LOG_VAR(var) \
	LogObject() << (var)->declarationFile << ":" << (var)->declarationLine << ": "

DefaultConfigSource::DefaultConfigSource()
{
	for (const ConfigVariable* var = ConfigVariable::head; var != NULL; var = var->next) {
		LOG_VAR(var) << "ConfigVariable var(\"" << var->key << "\", \"" << var->defaultValue << "\")\n";

		map<string, const ConfigVariable*>::const_iterator pos = vars.find(var->key);

		if (pos != vars.end()) {
			LOG_VAR(var) << "Duplicate config variable declaration \"" << var->key << "\"\n";
			LOG_VAR(pos->second) << "  Previously declared here\n";
		}
		else {
			vars[var->key] = var;
		}

		data[var->key] = var->defaultValue;
	}
}
