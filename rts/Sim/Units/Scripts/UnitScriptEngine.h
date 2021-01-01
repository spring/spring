/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* heavily based on CobEngine.h */

#ifndef UNIT_SCRIPT_ENGINE_H
#define UNIT_SCRIPT_ENGINE_H

#include <vector>

#include "System/creg/creg_cond.h"

struct UnitDef;
class CUnit;
class CUnitScript;


class CUnitScriptEngine
{
	CR_DECLARE_STRUCT(CUnitScriptEngine)

public:
	void AddInstance(CUnitScript* instance);
	void RemoveInstance(CUnitScript* instance);
	void ReloadScripts(const UnitDef* udef);

	void Tick(int deltaTime);

	void Init() { animating.reserve(256); }
	void Kill() { animating.clear(); }

	static void InitStatic();
	static void KillStatic();

private:
	CUnitScript* currentScript = nullptr;

	std::vector<CUnitScript*> animating;
};

extern CUnitScriptEngine* unitScriptEngine;

#endif // UNIT_SCRIPT_ENGINE_H
