#ifndef _KAIK_INCLUDE_H
#define _KAIK_INCLUDE_H


#include <ctime>
#include <fstream>
#include <deque>
#include <set>

#include <iostream>
#include <sstream>
#include <cassert>

#include "ExternalAI/Interface/aidefines.h" // SNPRINTF, STRCPY, ...


// Spring Standard Header
#include "System/StdAfx.h"

// Spring Component Registration System Headers
#include "creg/creg.h"
#include "creg/cregex.h"
#include "creg/Serializer.h"
#include "creg/STL_List.h"
#include "creg/STL_Map.h"


// Spring Engine Headers
#include "Sim/Units/UnitDef.h"					// Unit Definitions
#include "Sim/Units/CommandAI/CommandQueue.h"	// Unit Command Queues
#include "Sim/Features/FeatureDef.h"			// Feature Definitions
#include "Sim/MoveTypes/MoveInfo.h"				// Types of Movement units can have
#include "Sim/Weapons/WeaponDefHandler.h"		// Weapon Definitions

// Spring AI Interface Headers
#include "ExternalAI/aibase.h"					// DLL exports and definitions
#include "ExternalAI/IGlobalAI.h"				// Main AI file
#include "ExternalAI/IAICallback.h"				// Callback functions
#include "ExternalAI/IGlobalAICallback.h"		// AI Interface
#include "ExternalAI/IAICheats.h"				// AI Cheat Interface

// KAIK Headers
#include "Containers.h"							// All KAIK containers
#include "Defines.h"							// Definition declarations
#include "MTRand.h"								// Mersenne Twister RNG
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
#include "AttackGroup.h"
#include "AttackHandler.h"
#include "EconomyManager.h"
#include "DGunController.hpp"


#endif // _KAIK_
