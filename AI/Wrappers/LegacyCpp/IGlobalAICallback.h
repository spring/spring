/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IGLOBALAICALLBACK_H
#define IGLOBALAICALLBACK_H

#include <vector>
#include <deque>
#include "float3.h"
#include "Sim/Units/CommandAI/Command.h"

struct UnitDef;
struct FeatureDef;
class IAICheats;
class IAICallback;

/**
 * Each (legacy) C++ Skirmish AI receives one of these during initialization,
 * to be able to query things form the engine.
 */
class IGlobalAICallback
{
public:
	virtual IAICallback* GetAICallback()=0;
	/**
	 * Returns NULL if "/cheat" is not enabled, or there are several players
	 * in the game
	 */
	virtual IAICheats* GetCheatInterface()=0;

	// use virtual instead of pure virtual,
	// becuase pur evirtual is not well supported
	// among different OSs and compilers,
	// and pure virtual has no advantage
	// if we have other pure virtual functions
	// in the class
	virtual ~IGlobalAICallback() {}
};

#endif // IGLOBALAICALLBACK_H
