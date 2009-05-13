/* Author: Tobi Vollebregt */
/* heavily based on CobEngine.h */

#ifndef UNITSCRIPTENGINE_H
#define UNITSCRIPTENGINE_H

#include "LogOutput.h"

#include <list>

class CUnit;
class CUnitScript;


class CUnitScriptEngine
{
protected:
	std::list<CUnitScript*> animating;         //hash would be optimal. but not crucial.
	void CheckForDuplicates(const char* name, CUnitScript* instance);

public:
	CUnitScriptEngine(void);
	~CUnitScriptEngine(void);
	void AddInstance(CUnitScript* instance);
	void RemoveInstance(CUnitScript* instance);
	void Tick(int deltaTime);
};

extern CUnitScriptEngine GUnitScriptEngine;

#endif // UNITSCRIPTENGINE_H
