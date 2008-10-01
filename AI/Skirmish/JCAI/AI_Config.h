//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "CfgParser.h"

class MainAI;
class IGlobalAICallback;
class BuildTable;

class CfgBuildOptions : public CfgValue
{
public:
	struct BuildOpt 
	{
		BuildOpt () { def=0; count=0; info=0; }
		~BuildOpt () { if (info) delete info; }

		UnitDefID def;
		int count;
		string name;
		CfgList *info; // build requirements/conditions
	};
	
	void dbgPrint(int depth);
	bool Parse(CfgBuffer& buf);
	bool InitIDs ();

	vector <BuildOpt*> builds;
	int TotalBuilds ();
};

// Connects the CfgBuildOptions to the config parser
class CfgBuildOptionsType : public CfgValueClass
{
public:
	bool Identify(const CfgBuffer& buf) { return buf.CompareIdent ("buildoptions"); }
	CfgValue* Create() { return new CfgBuildOptions; }
};

extern CfgBuildOptionsType cfg_BuildOptionsType;


struct ForceConfig;
struct ReconConfig;

class UnitGroup;

// Holds general mod-independent config
class AIConfig 
{
public:
	AIConfig ();
	~AIConfig ();

	bool Load (const char *file, IGlobalAICallback *cb);

	bool cacheBuildTable;
	int infoblocksize;
	bool debug, showDebugWindow, showMetalSpots;

	int builderMoveTimeout;
	float builderMoveMinDistance;
	float threatDecay;

	int safeSectorRadius, mexSectorRadius;

	CfgList *root;
};

class ModConfig
{
public:
	ModConfig();
	~ModConfig();

	struct UnitTypeInfo {
		vector <UnitDefID> aadefense, gddefense;

		bool Load (CfgList *root, BuildTable *tbl);
	} unitTypesInfo;

	CfgList *root;

	bool Load (IGlobalAICallback *cb);
};


extern ModConfig modConfig;
extern AIConfig aiConfig; // general config
