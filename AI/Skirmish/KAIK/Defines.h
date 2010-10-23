#ifndef KAIK_DEFINES_HDR
#define KAIK_DEFINES_HDR

#include <cfloat>     // FLT_MAX, FLT_MIN

#include "AIExport.h" // for aiexport_getVersion()

#define AI_VERSION_NUMBER	aiexport_getVersion()
#define AI_NAME				std::string("KAIK ") + AI_VERSION_NUMBER + " Unofficial"
#define AI_DATE				__DATE__
#define AI_VERSION			AI_NAME + " (built " + AI_DATE + ")"
#define AI_CREDITS			"(developed by Krogothe, Tournesol, Firenu; now maintained by Kloot)"

// as of (at least) 58f99d175e79c02b1abc9be50d48325447cf7015
// float3::max{x,z}pos do not have their proper values when
// accessed in AI code, so float3::{Check,Is}InBounds() are
// both unreliable
#define MAPPOS_IN_BOUNDS(pos) \
	((pos.x >= 0) && (pos.x < ai->cb->GetMapWidth()  * SQUARE_SIZE)  && \
	 (pos.z >= 0) && (pos.z < ai->cb->GetMapHeight() * SQUARE_SIZE))

// Logger
#define L(ai, msg)      (ai->logger->Log(msg));

// Shortcuts
#define GCAT(a)         (ai->ut->GetCategory(a))

// RNGs
#define RANDINT         ai->math->RandInt()
#define RANDFLOAT       ai->math->MTRandFloat()

// Timer
#define TIMER_START     ai->math->TimerStart()
#define TIMER_TICKS     ai->math->TimerTicks()
#define TIMER_SECS      ai->math->TimerSecs()

// Folders
//    relative to "AI/Skirmish/KAIK/0.13"
#define ROOTFOLDER      ""

#define LOGFOLDER       std::string(ROOTFOLDER) + "logs/"
#define METALFOLDER     std::string(ROOTFOLDER) + "metal/"
#define CFGFOLDER       std::string(ROOTFOLDER) + "configs/"
#define CFGVERSION      1

// Error outputs
#define ERRORVECTOR     float3(-1.0f, 0.0f, 0.0f)

// Maths
#define MY_FLT_MAX      FLT_MAX
#define MY_FLT_MIN      FLT_MIN

#define DEG2RAD         0.01745329252f
#define RAD2DEG         57.2957795f

// Map sizing multipliers
#define METALMAP2MAPUNIT         2
#define MAPUNIT2POS              8
#define METALMAP2POS            16

// Threatmap / pathfinder resolution
#define THREATRES                8

// Maximum Builders helping each factory
#define MAXBUILDERSPERFACTORY    2
#define BUILDERFACTORYCOSTRATIO  0.5
// #define DEFENSEFACTORYRATIO   5
#define DEFENSEFACTORYRATIO      4

// Metal to energy ratio for cost calculations
#define METAL2ENERGY            45

// Minimum stocks for a "feasible" construction (ratio of storage)
#define FEASIBLEMSTORRATIO       0.3
#define FEASIBLEESTORRATIO       0.6

// Time idle units stay in limbo mode (in frames)
#define LIMBOTIME               40
// Income multiplier for tech tree advancement
#define INCOMEMULTIPLIER         5
// Seconds of storage to be had
#define STORAGETIME              6
// factory-feasibility ratio
#define ECONRATIO                0.85
// HACK: use only the last movetype
#define PATHTOUSE                ai->pather->NumOfMoveTypes - 1

// ClosestBuildsite Stuff
#define DEFCBS_SEPARATION        8
#define DEFCBS_RADIUS         2000

// Command lag acceptance 5 sec (30 * 5)
#define LAG_ACCEPTANCE         150

// SpotFinder stuff
#define CACHEFACTOR              8


// hub build-placement stuff
#define QUADRANT_TOP_LEFT        0
#define QUADRANT_TOP_RIGHT       1
#define QUADRANT_BOT_RIGHT       2
#define QUADRANT_BOT_LEFT        3
#define FACING_DOWN              0
#define FACING_RIGHT             1
#define FACING_UP                2
#define FACING_LEFT              3

#define MAX_NUKE_SILOS          16

// Unit categories
enum UnitCategory {
	CAT_COMM,
	CAT_ENERGY,
	CAT_MEX,
	CAT_MMAKER,
	CAT_BUILDER,
	CAT_ESTOR,
	CAT_MSTOR,
	CAT_FACTORY,
	CAT_DEFENCE,
	CAT_G_ATTACK,
	CAT_NUKE,
	// CAT_SHIELD,
	CAT_LAST
};

// UnitDef categories
enum UnitDefCategory {
	CAT_GROUND_FACTORY,
	CAT_GROUND_BUILDER,
	CAT_GROUND_ATTACKER,
	CAT_METAL_EXTRACTOR,
	CAT_METAL_MAKER,
	CAT_METAL_STORAGE,
	CAT_ENERGY_STORAGE,
	CAT_GROUND_ENERGY,
	CAT_GROUND_DEFENSE,
	CAT_NUKE_SILO
};

#endif
