//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#ifndef JC_BASE_AI_DEF_H
#define JC_BASE_AI_DEF_H

#include "GlobalStuff.h"
#include "ExternalAI/IGlobalAI.h"
#include "ExternalAI/IGlobalAICallback.h"
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include <assert.h> 
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <boost/bind.hpp>
#include <stdio.h>

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }

using namespace std;

#ifdef _MSC_VER
#pragma warning(disable: 4244 4018) // signed/unsigned and loss of precision...
#endif 

typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;

// outputs to testai.log
void logPrintf (const char *fmt, ...);
void logFileOpen ();
void logFileClose ();
void ChatMsgPrintf (IAICallback *cb, const char *fmt, ...);
void ChatDebugPrintf (IAICallback *cb, const char *fmt, ...);

void ReplaceExtension (const char *n, char *dst,int s, const char *ext);
class CfgList;
CfgList *LoadConfigFromFS (IAICallback *cb, const char *file);

#ifdef _MSC_VER
#define STRCASECMP stricmp
#define SNPRINTF _snprintf
#define VSNPRINTF _vsnprintf
#else
#define STRCASECMP strcasecmp
#define SNPRINTF snprintf
#define VSNPRINTF vsnprintf
#endif


#define NUM_TASK_TYPES 4
#define AI_PATH "aidll/globalai/jcai/"


struct ResourceInfo 
{
	ResourceInfo() { energy=metal=0.0f; } 
	ResourceInfo(float e,float m) : energy(e),metal(m) {}
	ResourceInfo& operator+=(const ResourceInfo &x) { energy+=x.energy; metal+=x.metal; return *this; }
	ResourceInfo& operator-=(const ResourceInfo &x) { energy-=x.energy; metal-=x.metal; return *this; }
	ResourceInfo operator+(const ResourceInfo &x) const { return ResourceInfo (energy+x.energy, metal+x.metal); }
	ResourceInfo& operator*=(const ResourceInfo &x) { energy*=x.energy;  metal*=x.metal; return *this; }
	ResourceInfo operator*(const ResourceInfo &x) const { return ResourceInfo (energy*x.energy, metal*x.metal); }
	ResourceInfo operator/(const ResourceInfo &x) const { return ResourceInfo (energy/x.energy, metal/x.metal); }
	ResourceInfo operator*(float x) const { return ResourceInfo (energy*x, metal*x); }
	float energy, metal;
};

class IAICallback;
class BuildMap;
class InfoMap;
class MetalSpotMap;
class CfgList;
class TaskManager;
class ResourceManager;
class TaskFactory;
class aiHandler;

// AI instance globals
class CGlobals
{
public:
	IAICallback *cb;
	BuildMap *buildmap;
	InfoMap *map;
	MetalSpotMap *metalmap;
	CfgList *sidecfg;
	TaskManager *taskManager;
	ResourceManager *resourceManager;
	set<aiHandler *> handlers;

	TaskFactory* taskFactories[NUM_TASK_TYPES];
};

#define IDBUF_SIZE 10000
extern int idBuffer[IDBUF_SIZE];

typedef int UnitDefID;

#endif // JC_BASE_AI_DEF_H

