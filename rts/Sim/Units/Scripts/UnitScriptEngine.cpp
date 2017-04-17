/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* heavily based on CobEngine.cpp */

#include "UnitScriptEngine.h"

#include "CobEngine.h"
#include "UnitScript.h"
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


/******************************************************************************/
/******************************************************************************/


CUnitScriptEngine::CUnitScriptEngine() : currentScript(nullptr)
{
}


CUnitScriptEngine::~CUnitScriptEngine()
{
}


void CUnitScriptEngine::CheckForDuplicates(const char* name, const CUnitScript* instance)
{
	int found = 0;

	for (const CUnitScript* us: animating) {
		if (us == instance)
			found++;
	}

	if (found > 1)
		LOG_L(L_WARNING, "%s found duplicates %d", name, found);
}


void CUnitScriptEngine::AddInstance(CUnitScript *instance)
{
	if (instance != currentScript)
		spring::VectorInsertUnique(animating, instance);
}


void CUnitScriptEngine::RemoveInstance(CUnitScript *instance)
{
	// Error checking
#ifdef _DEBUG
	CheckForDuplicates(__FUNCTION__, instance);
#endif

	//This is slow. would be better if instance was a hashlist perhaps
	if (instance != currentScript)
		spring::VectorErase(animating, instance);
}


void CUnitScriptEngine::Tick(int deltaTime)
{
	// Tick all instances that have registered themselves as animating
	int i = 0;
	while (i < animating.size()) {
		currentScript = animating[i];

		if (currentScript->Tick(deltaTime)) {
			++i;
		} else {
			animating[i] = animating.back();
			animating.pop_back();
		}
	}

	currentScript = nullptr;
}


/******************************************************************************/
/******************************************************************************/
