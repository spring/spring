#ifdef _MSC_VER
#pragma warning(disable: 4267  ) // signed/unsigned and loss of precision...4018,4244
#endif

class Global;

// Linux uses / but windows uses \\, however I believe it could be possible to have just /
// I'd need to test it
#ifdef WIN32
	#define slash "\\"
#endif
#ifndef WIN32
	#define _T(a) a
	#define slash "/"
#endif

#define EXCEPTION
// Exception handling

//#define TC_SOURCE
// adds a string to the command structures so that they can be tracked from their origin

//#define TNLOG
// Adds a Log command to the start of every function for huge logfiles so that errors and the flow of the program can
// be better tracked on remote computers without access to a debug build

// GAIA management macros
// used to filter out code based on whether it's set up as a gaia AI
#define ISGAIA (G->info->gaia == true)
#define ISNOTGAIA (G->info->gaia == false)
#define IF_GAIA if(G->info->gaia == true)
#define IF_NOTGAIA if(G->info->gaia == false)

#define GAIA_ONLY(a) {if(G->info->gaia == false) return a;}
#define NO_GAIA(a) {if(G->info->gaia == true) return a;}

// Used for macros where you dont wanna put something in but you cant use null/void
#define NA

// PI macros
// Used mainly in OTAI code that was ported, e.g. DT Ring Hnalding
#define PIx2                6.283185307L    // 360 Degrees
//#define PI                  3.141592654L    // 180 Degrees
#define PI_2                1.570796327L    //  90 Degrees
#define PI_4                0.785398163L    //  45 Degrees

// Game Time macros
//
// used as ( 4 SECONDS) remember to put brackets around otherwise a statement such as EVERY_(4 SECONDS) is interpreted as
// EVERY_(4) which is every 4 frames.
// This is all to simplify management of time using the 30 frames per second system.
#define SECONDS *30
#define SECOND *30
#define MINUTES *1800
#define MINUTE *1800
#define HOURS *108000
#define HOUR *108000
#define EVERY_SECOND (G->GetCurrentFrame()%30 == G->Cached->team)
#define EVERY_MINUTE (G->GetCurrentFrame()%1800 == G->Cached->team)
#define EVERY_(a) (G->GetCurrentFrame()%a == 0)
#define FRAMES

// converts from radians to degrees
#define DEGREES *(PI/180)

// Used to give end lines to the logger as endl cannot be passed to it
#define endline "\n"

// used to set B_IDLE as the type of a command
#define CMD_IDLE 86

// This makes the cached commands take a string as an extra argument to their constructor, allowing the
// string to be tracked by the debugger so you can tell where the command came from.
#ifdef TC_SOURCE
#define TCS(a,b) a(b)
#endif
#ifndef TC_SOURCE
#define TCS(a,b) a
#endif

// used to debug functions, functions log there names when called and the function that crashed the AI is the last one in the log listed
// CLOG is used before the logging class is initialized
// NLOG is used afterwards.
// CLOG relies on SendTxtMsg and is logged into infolog.txt and visible to the user
// NLOG only prints to the Log file
// Uncomment #defien TNLOG further up to enable this extensive logging
#ifdef TNLOG
#define NLOG(a) (G->L.header(string("function :: ") + a + string(endline)))
#define CLOG(a) {cb->SendTextMsg(a,1);}
#endif
#ifndef TNLOG
#define NLOG(a)
#define CLOG(a)
#endif
// Exception handling macros
#ifdef EXCEPTION
#define EXCEPTION_HANDLER(a,b,c) \
	try{ \
		a; \
	}catch(exception e){\
		c;\
		G->L.eprint(string("error in ")+b);\
		G->L.eprint(e.what());\
	}catch(exception* e){\
		c;\
		G->L.eprint(string("error in ")+b);\
		G->L.eprint(e->what());\
	}catch(string s){\
		c;\
		G->L.eprint(string("error in ")+b);\
		G->L.eprint(s);\
	}catch(...){\
		c;\
		G->L.eprint(string("error in ")+b);\
	}
#define EXCEPTION_MULTILINE_HANDLER(a,b,c) \
	try{ \
	a\
	}catch(exception e){\
	c;\
	G->L.eprint(string("error in ")+b);\
	G->L.eprint(e.what());\
	}catch(exception* e){\
	c;\
	G->L.eprint(string("error in ")+b);\
	G->L.eprint(e->what());\
	}catch(string s){\
	c;\
	G->L.eprint(string("error in ")+b);\
	G->L.eprint(s);\
	}catch(...){\
	c;\
	G->L.eprint(string("error in ")+b);\
	}
#else
#define EXCEPTION_HANDLER(a,b,c) a;
#define EXCEPTION_MULTILINE_HANDLER(a,b,c) a
#endif

// Used as  apart of TGA drawer and the Triangle drawing system which isnt currently used
struct ARGB{
	float blue;
	float green;
	float red;
	float alpha;
};

// The max n# of orders sent on each cycle by the command cache
#define BUFFERMAX 6

// typedefs to shorten these
typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
enum Ttarg{
	// Helps define targetting and random moves, but not used yet
	TARG_LAND,
	TARG_SEA,
	TARG_MIXED
};

enum Land_Type{
	// A vital part of the new terrain analysis and building placement system
	FLAT_LAND,
	WATER,
	METAL_PATCH,
	GEO_PATCH,
	UNBUILDABLE_LAND,
	PATHWAY_LAND,
	COAST
};
enum TEvent {
	// Not used atm, but will be a vital part of the new event messaging system
	E_UNIT_IDLE,
	E_UNIT_FINISHED,
	E_UNIT_CREATED,
	E_UNIT_DESTROYED,
	E_UNIT_MOVE_FAILED,
	E_UNIT_DAMAGED,
	E_UNIT_FIRED,
	E_UNIT_FRIENDLY_FIRE,
	E_UNIT_CAPTURED,
	E_CAPTURED_ENEMY,
	E_ENEMY_ENTER_LOS,
	E_ENEMY_LEFT_LOS,
	E_ENEMY_ENTER_RADAR,
	E_ENEMY_LEFT_RADAR,
	E_ENEMY_DESTROYED,
	E_ENEMY_FIRED,
	E_ENEMY_DAMAGED,
	E_COMMAND_SENT,
	E_COMMAND_FAILED,
	E_NA
};

enum btype {
	// The unviersal build tags, this helps define what a task is aka what it does. Very important
	B_POWER,
	B_MEX,
	B_RAND_ASSAULT,
	B_ASSAULT,
	B_FACTORY_CHEAP,
	B_FACTORY,
	B_BUILDER,
	B_GEO,
	B_SCOUT,
	B_RANDOM,
	B_DEFENCE,
	B_RADAR,
	B_ESTORE,
	B_TARG,
	B_MSTORE,
	B_SILO,
	B_JAMMER,
	B_SONAR,
	B_ANTIMISSILE,
	B_ARTILLERY,
	B_FOCAL_MINE,
	B_SUB,
	B_AMPHIB,
	B_MINE,
	B_CARRIER,
	B_METAL_MAKER,
	B_FORTIFICATION,
	B_DGUN,
	B_REPAIR,
	B_IDLE,
	B_CMD,
	B_RANDMOVE,
	B_RESURECT,
	B_GLOBAL,
	B_RULE,
	B_RULE_EXTREME,
	B_RULE_EXTREME_CARRY,
	B_RETREAT,
	B_GUARDIAN,
	B_BOMBER,
	B_SHIELD,
	B_MISSILE_UNIT,
	B_FIGHTER,
	B_GUNSHIP,
	B_GUARD_FACTORY,
	B_GUARD_LIKE_CON,
	B_RECLAIM,
	B_NA
};

enum t_priority {
	// Part of the command caching system, maybe this should be
	// removed as these values are ignored
	tc_instant,
	tc_pseudoinstant,
	tc_assignment,
	tc_low,
	tc_na
};

enum t_direction {// Used in Map->WhichCorner()
	t_NE,
	t_NW,
	t_SE,
	t_SW,
	t_NA
};

enum cdgun {// dont think this is actually used anymore
	cd_no,
	cd_yes,
	cd_unsure
};

enum unit_role{ // used for cosntruction untis to differentiate between factory and builder roles
	R_BUILDER,
	R_FACTORY,
	R_SCOUTER,
	R_ATTACKER,
	R_OTHER
};

// C++ headers
//#ifdef _MSC_VER
//#endif

// Standard libraries
#include <map>
#include <set>
#include <vector>
//#include <deque>
#include <time.h> // needed for logging adn random values
#include <fstream> // needed for logging and reading files


// Spring Engine
#include "GlobalStuff.h" // Common definitions in spring
#include "ExternalAI/IAICheats.h" // Cheat Interface
#include "ExternalAI/AICallback.h" // AI Callback
#include "ExternalAI/IGlobalAICallback.h" //GlobalAI callback
#include "Sim/Weapons/WeaponDefHandler.h" // Needed for WeaponDef
#include "Sim/Misc/FeatureDef.h" // Needed for FeatureDef

// helpers
#include "../Helpers/Log.h" // Logging class
#include "../Helpers/Terrain/CSector.h" // Map Sector data structure
#include "../Engine/TCommand.h" // Unit cached command data structure
//#include "../Events/EData.h" // Event information data structure
//#include "../Events/CEvent.h" // Event data structure
#include "../Core/CCached.h"// Cached data storage class
#include "../Engine/COrderRouter.h"// Caches orders and issues them so the engine doesnt give an overflow message
#include "../Helpers/CEconomy.h" // Construction rules
//#include "../Tasks/CTask.h" // Task Interface
//#include "../Tasks/CTaskFactory.h" // Task object generator/ Object Factory
//#include "../Helpers/Media/Bitmap.h" // Was used for loading Jpegs and bitmaps to draw onmap but it failed terribly
#include "../Helpers/Units/Actions.h" // Common actions in a useful class
#include "../Helpers/Terrain/Map.h" // Common Map related procedures such as which corner of the mapare we in
#include "../Helpers/TdfParser.h"// Parses TDF files
#include "../Helpers/Information.h" // Stores data from the mod.tdf and AI.tdf files
#include "../Helpers/InitUtil.h" // Seperates a string" a,b,c,d" in a vector<&T> {a,b,c,d}
#include "../Helpers/Terrain/RadarHandler.h" // Spaces out radar tower placement so they cover more area and dotn overlap
#include "../Helpers/Terrain/DTHandler.h" // Manages creation and pacement of DT rings
#include "../Helpers/Terrain/MetalMap.h" // Krogothes core metal spot fidner algorithm
#include "../Helpers/Terrain/MetalHandler.h" // Handles metal spots and choices
#include "../Helpers/ubuild.h" // Universal Build Routines

// Agents
#include "../Agents/Unit.h" // Task system for construction units
#include "../Agents/CBuilder.h" // Builder class, handles storage of tasks and cosntruction unit data
#include "../Agents/CManufacturer.h" // Loads buildtrees and drives construction processes through the Task Cycle
#include "../Agents/Planning.h" // Antistall algorithm and predictive targetting
//#include "../Agents/Factor.h" // Old cosntruction system, replaced by CManufacturer
#include "../Agents/Assigner.h" // A port of the metalmaker AI designed for Skirmish AI's, also handles cloaked units
#include "../Agents/Chaser.h" // Attack system.
#include "../Agents/Scouter.h" // Scouting system
