#pragma once


// Logger
#define L(a)			(*ai->LOGGER << a << endl)
#define LN(a)			(*ai->LOGGER << a)

// Shortcuts
#define GCAT(a)			(ai->ut->GetCategory(a))
#define GUG(a)			(ai->MyUnits[a]->groupID)

// RNGs
#define RANDINT ai->math->RandInt()
#define	RANDFLOAT ai->math->MTRandFloat()

// Timer
#define TIMER_START		ai->math->TimerStart()
#define TIMER_TICKS		ai->math->TimerTicks()
#define TIMER_SECS		ai->math->TimerSecs()
// Folders
#define ROOTFOLDER		"aidll\\globalai\\KAI\\"

#define LOGFOLDER		ROOTFOLDER"Logs\\"
#define	METALFOLDER		ROOTFOLDER"Metal\\"
#define TGAFOLDER		ROOTFOLDER"TGAs\\"

// KAI version to be appended to files generated. If older, remove them
#define KAIVERSION		1

// Error outputs
#define ERRORDEF		ai-cb->GetUnitDef("")
#define	ZEROVECTOR		float3(0,0,0)
#define	ERRORVECTOR		float3(-1,0,0)


// Maths
#define DEG2RAD		0.01745329252f
#define	RAD2DEG		57.2957795f



// Unit Categories
enum {CAT_COMM, CAT_ENERGY, CAT_MEX, CAT_MMAKER, CAT_BUILDER, CAT_ESTOR, CAT_MSTOR, CAT_FACTORY, CAT_DEFENCE, CAT_G_ATTACK, LASTCATEGORY};



//Map sizing multipliers
#define METALMAP2MAPUNIT	2
#define MAPUNIT2POS			8
#define	METALMAP2POS		16

// Threatmap / pathfinder resolution:
#define THREATRES			8

// Maximum Builders helping each factory
#define MAXBUILDERSPERFACTORY	2
#define BUILDERFACTORYCOSTRATIO	0.5
#define DEFENSEFACTORYRATIO		5

// Metal to energy ratio for cost calculations
#define METAL2ENERGY			45
// Minimum stocks for a "feasible" construction (ratio of storage)
#define	FEASIBLEMSTORRATIO		0.3
#define	FEASIBLEESTORRATIO		0.6
// Maximum units in a game
#define MAXUNITS				5000
// Time idle units stay in limbo mode (in frames)
#define LIMBOTIME				40
// Income multiplier for tech tree advancement
#define	INCOMEMULTIPLIER		5
// Seconds of storage to be had
#define STORAGETIME				6
// Think that your econ is this much for factory feasible
#define ECONRATIO				0.85
// Hacky stuff: use only one movetype:
#define PATHTOUSE				ai->pather->NumOfMoveTypes - 1

// ClosestBuildsite Stuff
#define	DEFCBS_SEPARATION	8
#define DEFCBS_RADIUS		2000

// Command lag acceptance 5 sec  (30 * 5)
#define LAG_ACCEPTANCE		150

// SpotFinder stuff
#define CACHEFACTOR			8

