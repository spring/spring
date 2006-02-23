#ifdef _MSC_VER
#pragma warning(disable: 4244 4018) // signed/unsigned and loss of precision...
#endif

class Global;
#define PI 3.141592654f
#define SQUARE_SIZE 8
#define GAME_SPEED 30
#define RANDINT_MAX 0x7fff
#define MAX_VIEW_RANGE 8000
#define MAX_PLAYERS 32
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
#define endline "\n"
#define LOG (G->L)
//#define TNLOG
#ifdef TNLOG
#define NLOG(a) (G->L.iprint(string("function :: ") + a))
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
#define BUFFERMAX 4
enum btype {B_POWER, B_MEX, B_RAND_ASSAULT, B_ASSAULT, B_FACTORY_CHEAP, B_FACTORY, B_BUILDER, B_GEO, B_SCOUT, B_RANDOM, B_DEFENCE, B_NA};
enum t_priority {tc_instant, tc_pseudoinstant, tc_assignment, tc_low};
enum t_direction {t_NE, t_NW, t_SE, t_SW};
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
#include "./Helpers/MetalMap.h"
#include "MetalHandler.h"
#include "SunParser.h"
#include "./Helpers/Log.h"
#include "UGroup.h"
#include "./Helpers/InitUtil.h"
#include "./Agents/Planning.h"
#include "Agents.h"
#include "./Agents/Assigner.h"
#include "./Agents/Chaser.h"
#include "./Agents/Scouter.h"

#define exprint(a) G->L.eprint(a)
