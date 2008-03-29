// Main include file for NTai
// from here all other headers are added
// Written by AF, under GPL V2

#ifdef _MSC_VER
	#pragma warning(disable: 4267  ) // signed/unsigned and loss of precision...4018,4244
#endif

namespace ntai{
	// Prototype the Global class
	class Global;
	
	// The name NTAI gives to the spring engine.
	const char AI_NAME[]= {"NTai XE9.79+"};
}

//using namespace ntai;

// C++ headers

// enumerations
#include "enums.h"


// pre-processor macros
#include "macros.h"


// Standard libraries

#include <cctype>
#include <stdexcept>

#include <set>
#include <ctime>		// Time
#include <fstream>		// File Streams

#include <deque>
#include <sstream>
#include <stdarg.h>

#include <stdio.h>
#include <assert.h>		// Assertions


// Boost headers

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

typedef boost::mutex mutex;
typedef boost::mutex::scoped_lock scoped_lock;

//using namespace std;


// engine includes
#include "../SDK/AI.h"										// AI interface includes

// helpers

#include "../Helpers/grid/CGridManager.h"					// Grid values/map class
#include "../Helpers/TdfParser.h"							// Parses TDF files
#include "../Helpers/mtrand.h"								// random number generator

#include "../Units/CUnitTypeData.h"							// Holder for unit type specific data
#include "../Core/CMessage.h"								// generic event message class
#include "../Core/IModule.h"								// Base class for AI objects
#include "../Units/IBehaviour.h"							// Behaviour base class

#include "../Units/ITaskManager.h"							// Attack nearby enemies
#include "../Units/ITaskManagerFactory.h"					// Attack nearby enemies
#include "../Units/CConfigTaskManager.h"							// Attack nearby enemies
#include "../Units/CUnit.h"
#include "../Helpers/Log.h"									// Logging class
#include "../Helpers/Units/CUnitDefLoader.h"				// Loads unitdefs
#include "../Engine/TCommand.h"								// Unit cached command data structure
#include "../Helpers/CWorkerThread.h"						// Some threading stuff

#include "../Core/CCached.h"								// Cached data storage class
#include "../Engine/COrderRouter.h"							// Caches orders and issues them so the engine doesnt give an overflow message
#include "../Helpers/CEconomy.h"							// Economy Construction rules

#include "../Helpers/Terrain/Map.h"							// Common Map related procedures such as which corner of the mapare we in

#include "../Helpers/CConfigData.h"							// Stores data from the mod.tdf and AI.tdf files
#include "../Helpers/CTokenizer.h"							// Tokenizes a string based on a delimiter aka String.split() in java
#include "../Helpers/Terrain/RadarHandler.h"				// Spaces out radar tower placement so they cover more area and dont overlap
#include "../Helpers/Terrain/DTHandler.h"					// Manages creation and pacement of DT rings
#include "../Helpers/Terrain/MetalMap.h"					// Krogothes core metal spot fidner algorithm
#include "../Helpers/Terrain/MetalHandler.h"				// Handles metal spots and choices
#include "../Helpers/Units/Actions.h"						// Common actions in a useful class
#include "../Helpers/ubuild.h"								// Universal Build Routines
#include "../Helpers/Terrain/CBuildingPlacer.h"				// Building placement algorithm


// Agents

#include "../Agents/CManufacturer.h"						// Loads buildtrees and drives construction processes through the Task Cycle
#include "../Agents/Planning.h"								// Antistall algorithm and predictive targetting
#include "../Agents/Chaser.h"								// Attack system.


// Unit Tasks

#include "../Tasks/CUnitConstructionTask.h"					// A task for building things by name
#include "../Tasks/CConsoleTask.h"							// Outputs a message to the chat console
#include "../Tasks/CKeywordConstructionTask.h"				// Handles a universal build keyword/action
#include "../Tasks/CLeaveBuildSpotTask.h"					// Makes units walk forward out of factories

// Unit behaviours

#include "../Units/Behaviours/AttackBehaviour.h"			// Attack nearby enemies
#include "../Units/Behaviours/CDGunBehaviour.h"				// DGun routines
#include "../Units/Behaviours/MetalMakerBehaviour.h"		// Metal maker efficiency routines
#include "../Units/Behaviours/CRetreatBehaviour.h"			// Retreat when damaged
#include "../Units/Behaviours/CKamikazeBehaviour.h"			// Self destruct when near to enemies
#include "../Units/Behaviours/CStaticDefenceBehaviour.h"	// Attack units within firing range
#include "../Units/Behaviours/CMoveFailReclaimBehaviour.h"	// Issue an area reclaim command when stuck

// Global classes

#include "Global.h"											// (the root object representing the AI itself)

	

