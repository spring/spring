/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* heavily based on CobEngine.cpp */

#include "UnitScriptEngine.h"
#include "UnitScript.h"
#include "UnitScriptLog.h"

#include "System/FileSystem/FileHandler.h"

#ifndef _CONSOLE
	#include "System/TimeProfiler.h"
#else
	#define START_TIME_PROFILE(a) {}
	#define END_TIME_PROFILE(a) {}
	#define SCOPED_TIMER(a) {}
#endif



CUnitScriptEngine GUnitScriptEngine;


/******************************************************************************/
/******************************************************************************/


CUnitScriptEngine::CUnitScriptEngine() : currentScript(NULL)
{
}


CUnitScriptEngine::~CUnitScriptEngine()
{
}


void CUnitScriptEngine::CheckForDuplicates(const char* name, CUnitScript* instance)
{
	int found = 0;

	for (std::list<CUnitScript*>::iterator i = animating.begin(); i != animating.end(); ++i) {
		if (*i == instance)
			found++;
	}

	if (found > 1)
		LOG_L(L_WARNING, "%s found duplicates %d", name, found);
}


void CUnitScriptEngine::AddInstance(CUnitScript *instance)
{
	if (instance != currentScript)
		animating.push_front(instance);

	// Error checking
#ifdef _DEBUG
	CheckForDuplicates(__FUNCTION__, instance);
#endif
}


void CUnitScriptEngine::RemoveInstance(CUnitScript *instance)
{
	// Error checking
#ifdef _DEBUG
	CheckForDuplicates(__FUNCTION__, instance);
#endif

	//This is slow. would be better if instance was a hashlist perhaps
	if (instance != currentScript)
		animating.remove(instance);
}


void CUnitScriptEngine::Tick(int deltaTime)
{
	SCOPED_TIMER("UnitScriptEngine::Tick");

	// Tick all instances that have registered themselves as animating
	for (std::list<CUnitScript*>::iterator it = animating.begin(); it != animating.end(); ) {
		currentScript = *it;

		if (currentScript->Tick(deltaTime)) {
			++it;
		} else {
			it = animating.erase(it);
		}
	}

	currentScript = NULL;
}


/******************************************************************************/
/******************************************************************************/
