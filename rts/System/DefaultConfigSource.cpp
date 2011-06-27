/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "DefaultConfigSource.h"
#include "ConfigVariable.h"
#include "LogOutput.h"

using std::map;
using std::string;

static LogObject& logVar(const ConfigVariable* var)
{
	static LogObject log;
	return log << var->declarationFile << ":" << var->declarationLine << ": ";
}

DefaultConfigSource::DefaultConfigSource()
{
	for (const ConfigVariable* var = ConfigVariable::head; var != NULL; var = var->next) {
		//LogObject() << var->declarationFile << ":" << var->declarationLine << ": ConfigVariable var(\"" << var->key << "\", \"" << var->defaultValue << "\")\n";

		map<string, const ConfigVariable*>::const_iterator pos = vars.find(var->key);

		if (pos != vars.end()) {
			logVar(var) << "Duplicate config variable declaration \"" << var->key << "\"\n";
			logVar(pos->second) << "  Previously declared here\n";
		}
		else {
			vars[var->key] = var;
		}

		data[var->key] = var->defaultValue;
	}
}
