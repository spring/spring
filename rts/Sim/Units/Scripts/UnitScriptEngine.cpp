/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* heavily based on CobEngine.cpp */

#include "UnitScriptEngine.h"

#include "CobEngine.h"
#include "UnitScript.h"
#include "UnitScriptFactory.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Util.h"
#include "System/FileSystem/FileHandler.h"

CUnitScriptEngine* unitScriptEngine = nullptr;


CR_BIND(CUnitScriptEngine, )

CR_REG_METADATA(CUnitScriptEngine, (
	CR_MEMBER(animating),

	//always null when saving
	CR_IGNORED(currentScript)
))


void CUnitScriptEngine::InitStatic() {
	cobEngine = new CCobEngine();
	cobFileHandler = new CCobFileHandler();
	unitScriptEngine = new CUnitScriptEngine();
}
void CUnitScriptEngine::KillStatic() {
	spring::SafeDelete(cobEngine);
	spring::SafeDelete(cobFileHandler);
	spring::SafeDelete(unitScriptEngine);
}



void CUnitScriptEngine::ReloadScripts(const UnitDef* udef)
{
	const CCobFile* oldScript = cobFileHandler->GetScriptAddr(udef->scriptName);

	if (oldScript == nullptr) {
		LOG_L(L_WARNING, "[UnitScriptEngine::%s] unknown COB script for unit \"%s\": %s", __func__, udef->name.c_str(), udef->scriptName.c_str());
		return;
	}

	CCobFile* newScript = cobFileHandler->ReloadCobFile(udef->scriptName);

	if (newScript == nullptr) {
		LOG_L(L_WARNING, "[UnitScriptEngine::%s] could not load COB script for unit \"%s\" from: %s", __func__, udef->name.c_str(), udef->scriptName.c_str());
		return;
	}

	int count = 0;
	for (size_t i = 0; i < unitHandler->MaxUnits(); i++) {
		CUnit* unit = unitHandler->GetUnit(i);

		if (unit == nullptr)
			continue;

		CCobInstance* cob = dynamic_cast<CCobInstance*>(unit->script);

		if (cob == nullptr || cob->GetScriptAddr() != oldScript)
			continue;

		count++;

		spring::SafeDestruct(unit->script);

		unit->script = CUnitScriptFactory::CreateCOBScript(unit, newScript);
		unit->script->Create();
	}

	LOG("[UnitScriptEngine::%s] reloaded COB scripts for %i units", __func__, count);
}

void CUnitScriptEngine::CheckForDuplicates(const char* name, const CUnitScript* instance)
{
	int found = 0;

	for (const CUnitScript* us: animating) {
		found += (us == instance);
	}

	if (found > 1)
		LOG_L(L_WARNING, "[UnitScriptEngine::%s] %d duplicates found for %s", __func__, found, name);
}


void CUnitScriptEngine::AddInstance(CUnitScript* instance)
{
	if (instance == currentScript)
		return;

	spring::VectorInsertUnique(animating, instance);
}


void CUnitScriptEngine::RemoveInstance(CUnitScript* instance)
{
	// Error checking
#ifdef _DEBUG
	CheckForDuplicates(__FUNCTION__, instance);
#endif

	//This is slow. would be better if instance was a hashlist perhaps
	if (instance == currentScript)
		return;

	spring::VectorErase(animating, instance);
}


void CUnitScriptEngine::Tick(int deltaTime)
{
	cobEngine->Tick(deltaTime);

	// Tick all instances that have registered themselves as animating
	size_t i = 0;
	while (i < animating.size()) {
		currentScript = animating[i];

		if (!currentScript->Tick(deltaTime)) {
			animating[i] = animating.back();
			animating.pop_back();
			continue;
		}

		i++;
	}

	currentScript = nullptr;
}

