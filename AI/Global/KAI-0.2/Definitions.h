#ifndef DEFINITIONS_H
#define DEFINITIONS_H


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
#define ROOTFOLDER		"AI/KAI/"

#define LOGFOLDER		ROOTFOLDER"Logs/"
#define	METALFOLDER		ROOTFOLDER"Metal/"
#define TGAFOLDER		ROOTFOLDER"TGAs/"

// Error outputs
#define ERRORDEF		ai-cb->GetUnitDef("")
#define	ZEROVECTOR		float3(0,0,0)
#define	ERRORVECTOR		float3(-1,0,0)


// Maths
#define DEG2RAD		0.01745329252f
#define	RAD2DEG		57.2957795f



// Unit Categories
enum {CAT_COMM, CAT_ENERGY, CAT_MEX, CAT_MMAKER, CAT_BUILDER, CAT_ESTOR, CAT_MSTOR, CAT_FACTORY, CAT_DEFENCE, CAT_ATTACK, CAT_ARTILLERY, CAT_ASSAULT, CAT_A_ATTACK, CAT_TRANSPORT, CAT_A_TRANSPORT, CAT_RADAR, CAT_SONAR, CAT_R_JAMMER, CAT_S_JAMMER, CAT_LONG_RANGE_WEAPON, CAT_NUKE, CAT_ANTINUKE, CAT_SHIELD, LASTCATEGORY};

// The bit mask category system:

// Economy stuff:
// An factory can make units or buildings (so an old factory will be 'CATM_FACTORY | CATM_BUILDING' )
#define CATM_FACTORY	1
// The unit makes a total surplus of energy
#define CATM_ENERGY		2
// The unit needs/uses ground metal
#define CATM_MEX		4
// The unit makes a total surplus of metal (it might need energy to do so)
#define CATM_MMAKER		8
// A builder can help build stuff
#define CATM_BUILDER	16
// The unit can store energy (no efficiency testing is made)
#define CATM_ESTOR		32
// The unit can store metal (no efficiency testing is made)
#define CATM_MSTOR		64
// This unit needs a geo to be made, it might be a bad idea to have it here...
#define CATM_GEO		128

// Battle stuff:
// In the old category system defence was buildings only, but here it might mean other units too (TODO: needs many changes if so)
// Its still have the same meaning atm however.
#define CATM_DEFENCE	256
// An moveable unit with weapons
#define CATM_ATTACK		512
// This is an artillery unit, intended to be handled differently from other attack/defense units (TODO: needs normalized range data. Might not belong here)
#define CATM_LONGRANGE	1024
// The unit can make/fire nuks
#define CATM_NUKE		2048
// The unit can make/fire anti nuks
#define CATM_ANTINUKE	4096
// The unit can make an aoe shield
#define CATM_SHIELD		8192

// Unit build/move stuff:
// The unit can be on ground
#define CATM_GROUND		16384
// The unit can be on water
#define CATM_WATER		32768
// The unit can fly
#define CATM_AIR		65536
// Usefull for making lists:
#define CATM_BUILDING	268435456
// Usefull for making lists:
#define CATM_CANMOVE	536870912

// Unit reconnaissance stuff: 
// Has long vision (relative to other units or relative to its cost)
#define CATM_SCOUT		131072
// The unit is not seen on radar or sonar
#define CATM_STEALTH	262144
// The unit can hide from vision
#define CATM_CLOAK		524288
// The unit have a radar
#define CATM_RADAR 		1048576
// The unit have a sonar
#define CATM_SONAR		2097152
// The unit have a sonar jammer
#define CATM_S_JAMMER	4194304
// The unit have a radar jammer
#define CATM_R_JAMMER	8388608

// Special units
// The unit can transport other units
#define CATM_TRANSPORT	16777216
// The unit will self destruct if other units comes close
#define CATM_KAMIKAZE	33554432
// The unit is marked as a 'commander' by spring (this category might be useless)
#define CATM_COMM		67108864
// Its a feature (dragon teeth++), might be used for terraforming or base defense
#define CATM_FEATURE	134217728
// Its a carrier/repair pad
#define CATM_REPAIRPAD	1073741824
// Unused:
#define CATM_32			0x80000000
 
//Map sizing multipliers
#define METALMAP2MAPUNIT	2
#define MAPUNIT2POS			8
#define	METALMAP2POS		16

// Threatmap / pathfinder resolution:
#define THREATRES			8
// Ignore this many closest cells in the chokepoint map (so no chokepoints are found at comm spawn due to high traffic)
#define CHOKEPOINTMAP_IGNORE_CELLS 12

// Maximum Builders helping each factory
#define MAXBUILDERSPERFACTORY	6
#define BUILDERFACTORYCOSTRATIO	0.5
#define MINIMUMDEFENSEFACTORYRATIO 4 
#define DEFENSEFACTORYRATIO		8
#define MAXIMUMDEFENSEFACTORYRATIO		20
#define ATTACKERSPERBUILDER		3
#define FACTORYBUILDTIMEOUT		10

// Metal to energy ratio for cost calculations
#define METAL2ENERGY			45
// Minimum stocks for a "feasible" construction (ratio of storage)
#define	FEASIBLEMSTORRATIO		0.4
#define	FEASIBLEESTORRATIO		0.6
// Maximum units in a game
#define MAXUNITS				MAX_UNITS//10000
// Time idle units stay in limbo mode (in frames)
#define LIMBOTIME				40
// Income multiplier for tech tree advancement
#define	INCOMEMULTIPLIER		5
// Seconds of storage to be had
#define STORAGETIME				10
// Think that your econ is this much for factory feasible
#define ECONRATIO				0.85
// Hacky stuff: use only one movetype:
#define PATHTOUSE				1 << (ai->pather->NumOfMoveTypes - 1)
#define AIRPATH					0x80000000

// ClosestBuildsite Stuff
#define	DEFCBS_SEPARATION	8
#define DEFCBS_RADIUS		2000

// Command lag acceptance 5 sec  (30 * 5)
#define LAG_ACCEPTANCE		150

// SpotFinder stuff
#define CACHEFACTOR			8


#endif /* DEFINITIONS_H */
