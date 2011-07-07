/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* heavily based on CobEngine.cpp */

#include "UnitScriptEngine.h"
#include "UnitScript.h"

#include "LogOutput.h"
#include "FileSystem/FileHandler.h"

#ifndef _CONSOLE
	#include "System/TimeProfiler.h"
#else
	#define START_TIME_PROFILE(a) {}
	#define END_TIME_PROFILE(a) {}
	#define SCOPED_TIMER(a) {}
#endif

#include "mmgr.h"


CUnitScriptEngine GUnitScriptEngine;


/******************************************************************************/
/******************************************************************************/


CUnitScriptEngine::CUnitScriptEngine()
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
		logOutput.Print("Warning: %s found duplicates %d", name, found);
}


void CUnitScriptEngine::AddInstance(CUnitScript *instance)
{
	animating.push_front(instance);

	// Error checking
	//CheckForDuplicates(__FUNCTION__, instance);
}


void CUnitScriptEngine::RemoveInstance(CUnitScript *instance)
{
	// Error checking
	//CheckForDuplicates(__FUNCTION__, instance);

	//This is slow. would be better if instance was a hashlist perhaps
	animating.remove(instance);
}


void CUnitScriptEngine::Tick(int deltaTime)
{
	SCOPED_TIMER("UnitScriptEngine::Tick");

	// Tick all instances that have registered themselves as animating
	for (std::list<CUnitScript*>::iterator it = animating.begin(); it != animating.end(); ) {
		if ((*it)->Tick(deltaTime) == -1)
			it = animating.erase(it);
		else
			++it;
	}
}


/******************************************************************************/
/******************************************************************************/
