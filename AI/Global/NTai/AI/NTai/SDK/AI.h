//-------------------------------------------------------------------------
// NTai
// Copyright 2004-2007 AF
// Released under GPL 2 license
//-------------------------------------------------------------------------

// typedefs to shorten these
typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;

// STD includes required
#include <vector>
#include <map>
#include <string>
#include <deque>
#include <list>

#include <functional>
#include <algorithm>
#include <locale>


// Spring Engine

#include "IGlobalAI.h" // Interface spring uses to make AI callouts
#include "ExternalAI/IAICallback.h" // AI Callback
#include "ExternalAI/IGlobalAICallback.h" //GlobalAI callback
#include "ExternalAI/IAICheats.h" // Cheat Interface
#include "GlobalStuff.h" // Common definitions in spring
#include "Sim/Weapons/WeaponDefHandler.h" // Needed for WeaponDef
#include "Sim/Misc/DamageArray.h" // Needed for WeaponDef
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Features/FeatureDef.h" // Needed for FeatureDef


/*#include "creg.h" //CREG
#include "float3.h" // map positions
#include "aibase.h" // Cheat Interface
#include "CommandQueue.h"
#include "IAICallback.h" // AI Callback
#include "IAICheats.h" // Cheat Interface
#include "IGlobalAICallback.h" //GlobalAI callback
#include "IGlobalAI.h" // IGlobalAI
#include "GlobalStuff.h" // Common definitions in spring
#include "MoveInfo.h"
#include "FeatureDef.h" // Needed for FeatureDef
*/
