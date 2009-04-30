/* Author: Tobi Vollebregt */
/* heavily based on CobEngine.h */

#ifndef UNITSCRIPTENGINE_H
#define UNITSCRIPTENGINE_H

#include "LogOutput.h"

#include <list>
#include <queue>
#include <map>

class CUnitScript;


class CUnitScriptEngine
{
protected:
	std::list<CUnitScript *> animating;         //hash would be optimal. but not crucial.
	void CheckForDuplicates(const char* name, CUnitScript *instance);
public:
	CUnitScriptEngine(void);
	~CUnitScriptEngine(void);
	void AddInstance(CUnitScript *instance);
	void RemoveInstance(CUnitScript *instance);
	void Tick(int deltaTime);
	void ShowScriptWarning(const std::string& msg);
	void ShowScriptError(const std::string& msg);
};

extern CUnitScriptEngine GUnitScriptEngine;

#endif // UNITSCRIPTENGINE_H
