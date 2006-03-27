#ifdef _MSC_VER
#pragma warning(disable: 4267 4244 4018) // signed/unsigned and loss of precision...
#endif

class Global;
#ifdef WIN32
	#define slash _T("\\")
        #include <tchar.h>       //<----move tchar here for windows only
#endif
#ifndef WIN32
	#define slash _T("/")
        #define _T(x) x         //<-----replacement macro
#endif

//#define _T(x)       __T(x)
//#define _TEXT(x)    __T(x)

#define ISGAIA (G->gaia == true)
#define ISNOTGAIA (G->gaia == false)
#define IF_GAIA if(G->gaia == true)
#define IF_NOTGAIA if(G->gaia == false)

#define GAIA_ONLY {if(G->gaia == false) return;}
#define NO_GAIA {if(G->gaia == true) return;}

//#define PI 3.141592654f
//#define SQUARE_SIZE 8
//#define GAME_SPEED 30
//#define RANDINT_MAX 0x7fff
//#define MAX_PLAYERS 32

#define SECONDS *30
#define SECOND *30
#define MINUTES *1800
#define MINUTE *1800
#define HOURS *108000
#define HOUR *108000
#define EVERY_SECOND (G->cb->GetCurrentFrame()%30 == G->team)
#define EVERY_MINUTE (G->cb->GetCurrentFrame()%1800 == G->team)
#define EVERY_(a) (G->cb->GetCurrentFrame()%a == 0)
#define FRAMES

#define DEGREES *(PI/180)

typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;

#define endline _T("\n")
#define LOG  (G->L)

// TC_SOURCE

#ifdef TC_SOURCE
#define TCS(a,b) a(b)
#endif
#ifndef TC_SOURCE
#define TCS(a,b) a
#endif

//#define TNLOG

#ifdef TNLOG
#define NLOG(a) (G->L.iprint(string(_T("function :: ")) + a))
#endif
#ifndef TNLOG
#define NLOG(a)
#endif
	
struct ARGB{
	float blue;
	float green;
	float red;
	float alpha;
};

#define BUFFERMAX 5

enum btype {
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
	B_MSTORE,
	/*B_SILO,
	B_JAMMER,
	B_SONAR,
	B_ANTIMISSILE,
	B_ARTILLERY,
	B_FOCAL_MINE,
	B_SUB,
	B_AMPHIB,*/
	B_MINE,
	B_CARRIER,
	B_METAL_MAKER,
	B_FORTIFICATION,
	B_DGUN,
	B_REPAIR,
	B_NA
};

enum t_priority {
	tc_instant,
	tc_pseudoinstant,
	tc_assignment,
	tc_low,
	tc_na
};

enum t_direction {
	t_NE,
	t_NW,
	t_SE,
	t_SW
};

#ifdef _MSC_VER
#include <process.h>
#endif
#include <map>
#include <set>
#include <vector>
#include <deque>
#include <time.h>
#include <fstream>

#include "GlobalStuff.h"
#include "ExternalAI/IAICallback.h"
#include "ExternalAI/IGlobalAICallback.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Misc/FeatureDef.h"

#include "Helpers/InitUtil.h"
#include "./Helpers/MetalMap.h"
#include "Helpers/MetalHandler.h"
#include "Helpers/SunParser.h"
#include "./Helpers/Log.h"

#include "Agents/Unit.h"
#include "Agents/Planning.h"
#include "Agents/Factor.h"
#include "Agents/Assigner.h"
#include "Agents/Chaser.h"
#include "Agents/Scouter.h"

#define exprint(a) G->L.eprint(a)
