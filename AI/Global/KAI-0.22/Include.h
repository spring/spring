#ifndef INCLUDE_H
#define INCLUDE_H



#include <ctime>		// Time
#include <fstream>		// File Streams
#include <algorithm>
#include <deque>
#include <sstream>
#include <stdarg.h>
#include <functional>
#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>

#ifdef WIN32
#include <direct.h>		// Folder manipulation
#include <io.h>
//#include <windows.h>
#else
#include <sys/time.h>
#define LARGE_INTEGER struct timeval
static inline void itoa(int i, char* buf, int size) {
        snprintf(buf, size, "%d", i);
}
#endif


// Spring Standard Header
#include "./System/StdAfx.h"

// Spring Engine
#include "./Game/command.h"                     // Commands
#include "./Sim/Units/UnitDef.h"                // Unit Definitions
#include "./Sim/Misc/FeatureDef.h"            // Feature Definitions
#include "./Sim/MoveTypes/MoveInfo.h"           // Types of Movement units can have
#include "./Sim/Weapons/WeaponDefHandler.h"		// Weapon Definitions
//#include "float3.h"								// Float3 container and operators

// Spring AI
#include "ExternalAI/aibase.h"					// DLL exports and definitions
#include "ExternalAI/IGlobalAI.h"				// Main AI file
#include "ExternalAI/IAICallback.h"				// Callback functions
#include "ExternalAI/IGlobalAICallback.h"		// AI Interface
#include "ExternalAI/IAICheats.h"				// AI Cheat Interface

// KAI
#include "Containers.h"							// All KAI containers
#include "Definitions.h"						// Definition declarations
#include "MTRand.h"								// Mersenne Twister RNG
#include "SurveillanceHandler.h"
#include "SpotFinder.h"
#include "MicroPather.h"
#include "Maths.h"
#include "SunParser.h"
#include "MetalMap.h"
#include "Debug.h"
#include "PathFinder.h"
#include "UnitTable.h"
#include "ThreatMap.h"
#include "UnitHandler.h"
#include "Unit.h"
#include "EconomyTracker.h"
#include "DefenseMatrix.h"
#include "BuildUp.h"
#include "AttackHandler.h"
#include "AttackGroup.h"
#include "EconomyManager.h"
#include "DamageControl.h"




            



#endif /* INCLUDE_H */
