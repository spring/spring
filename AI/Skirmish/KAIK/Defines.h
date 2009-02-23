#ifndef DEFINES_H
#define DEFINES_H

#include "AIExport.h" // for aiexport_getVersion() and aiexport_getDataDir()

#define AI_VERSION_NUMBER aiexport_getVersion()
//#define AI_VERSION_NUMBER "0.13"
#define AI_NAME			std::string("KAIK ") + AI_VERSION_NUMBER + " Unofficial"
#define AI_DATE			"20/10/2008"
#define AI_VERSION		AI_NAME + " (rev. " + AI_DATE + ")"
#define AI_CREDITS		"(original developer: Krogothe, current maintainer: Kloot)"

// Logger
#define L(a)			(*ai->LOGGER << a << std::endl)
#define LN(a)			(*ai->LOGGER << a)

// Shortcuts
#define GCAT(a)			(ai->ut->GetCategory(a))
#define GUG(a)			(ai->MyUnits[a]->groupID)

// RNGs
#define RANDINT			ai->math->RandInt()
#define RANDFLOAT		ai->math->MTRandFloat()

// Timer
#define TIMER_START		ai->math->TimerStart()
#define TIMER_TICKS		ai->math->TimerTicks()
#define TIMER_SECS		ai->math->TimerSecs()

// Folders
#define ROOTFOLDER		aiexport_getDataDir(false)

#define LOGFOLDER		std::string(ROOTFOLDER) + "Logs/"
#define METALFOLDER		std::string(ROOTFOLDER) + "Metal/"
#define TGAFOLDER		std::string(ROOTFOLDER) + "TGAs/"
#define CFGFOLDER		std::string(ROOTFOLDER) + "CFGs/"

// Error outputs
#define ZEROVECTOR		float3( 0, 0, 0)
#define ERRORVECTOR		float3(-1, 0, 0)

// Maths
#define DEG2RAD			0.01745329252f
#define RAD2DEG			57.2957795f


// Unit Categories
enum {
	CAT_COMM, CAT_ENERGY, CAT_MEX, CAT_MMAKER,
	CAT_BUILDER, CAT_ESTOR, CAT_MSTOR, CAT_FACTORY,
	CAT_DEFENCE, CAT_G_ATTACK, CAT_NUKE, /* CAT_SHIELD, */
	LASTCATEGORY
};



// Map sizing multipliers
#define METALMAP2MAPUNIT		 2
#define MAPUNIT2POS				 8
#define METALMAP2POS			16

// Threatmap / pathfinder resolution
#define THREATRES				8

// Maximum Builders helping each factory
#define MAXBUILDERSPERFACTORY	2
#define BUILDERFACTORYCOSTRATIO	0.5
// #define DEFENSEFACTORYRATIO		5
#define DEFENSEFACTORYRATIO		4

// Metal to energy ratio for cost calculations
#define METAL2ENERGY			45

// Minimum stocks for a "feasible" construction (ratio of storage)
#define FEASIBLEMSTORRATIO		0.3
#define FEASIBLEESTORRATIO		0.6

// Time idle units stay in limbo mode (in frames)
#define LIMBOTIME				40
// Income multiplier for tech tree advancement
#define INCOMEMULTIPLIER		5
// Seconds of storage to be had
#define STORAGETIME				6
// Think that your econ is this much for factory feasible
#define ECONRATIO				0.85
// Hacky stuff: use only one movetype
#define PATHTOUSE				ai->pather->NumOfMoveTypes - 1

// ClosestBuildsite Stuff
#define DEFCBS_SEPARATION		8
#define DEFCBS_RADIUS			2000

// Command lag acceptance 5 sec (30 * 5)
#define LAG_ACCEPTANCE			150

// SpotFinder stuff
#define CACHEFACTOR				8


// hub build-placement stuff
#define QUADRANT_TOP_LEFT	0
#define QUADRANT_TOP_RIGHT	1
#define QUADRANT_BOT_RIGHT	2
#define QUADRANT_BOT_LEFT	3
#define FACING_DOWN			0
#define FACING_RIGHT		1
#define FACING_UP			2
#define FACING_LEFT			3

#define MAX_NUKE_SILOS		16


#endif
