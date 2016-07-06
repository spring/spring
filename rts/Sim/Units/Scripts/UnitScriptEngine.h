/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* heavily based on CobEngine.h */

#ifndef UNIT_SCRIPT_ENGINE_H
#define UNIT_SCRIPT_ENGINE_H

#include "System/creg/creg_cond.h"
#include <vector>

class CUnit;
class CUnitScript;


class CUnitScriptEngine
{
	CR_DECLARE_STRUCT(CUnitScriptEngine)

protected:
	std::vector<CUnitScript*> animating; // hash would be optimal, but not crucial
	void CheckForDuplicates(const char* name, const CUnitScript* instance);

public:
	CUnitScriptEngine();
	~CUnitScriptEngine();
	void AddInstance(CUnitScript* instance);
	void RemoveInstance(CUnitScript* instance);
	void Tick(int deltaTime);

	static void InitStatic();
	static void KillStatic();
private:
	CUnitScript* currentScript;
};

extern CUnitScriptEngine* unitScriptEngine;

#endif // UNIT_SCRIPT_ENGINE_H
