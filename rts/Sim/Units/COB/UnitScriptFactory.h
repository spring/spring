/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITSCRIPTFACTORY_H
#define UNITSCRIPTFACTORY_H

#include <string>

class CUnit;
class CUnitScript;

class CUnitScriptFactory
{
public:
	static CUnitScript* CreateScript(const std::string& name, CUnit* unit);
};

#endif
