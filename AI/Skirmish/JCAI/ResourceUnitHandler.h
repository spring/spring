//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "ptrvec.h"

class CfgBuildOptions;

// Resource units are units that produce energy/metal
class ResourceUnit : public aiUnit
{
public:
	ResourceUnit() {index=0; turnedOn=true;}

	bool turnedOn; // for metal maker control
	int index;
};

class ExtractorUnit : public ResourceUnit
{
public:
	int spot;
};

struct RUHandlerConfig
{
	RUHandlerConfig() { EnergyBuildRatio=MetalBuildRatio=0.0f; }

	bool Load (CfgList *sidecfg);

	float EnergyBuildRatio;
	float MetalBuildRatio;

	struct EnergyBuildHeuristicConfig
	{
		float EnergyCost,MetalCost,BuildTime;
		float MaxUpscale;

		void Load (CfgList *c);
	} EnergyHeuristic;

	struct MetalBuildHeuristicConfig
	{
		float PaybackTimeFactor;
		float EnergyUsageFactor;
		float ThreatConversionFactor;
		float PrefUpscale;
		float UpscaleOvershootFactor;

		void Load (CfgList *c);
	} MetalHeuristic;

	// Metal production enable policy, based on the metal maker AI
	struct {
		float MinUnitEnergyUsage;
		float MinEnergy;
		float MaxEnergy;
	} EnablePolicy;

	struct {
		vector <int> MetalStorage;
		vector <int> EnergyStorage;
		float MaxRatio;
		float MinMetalIncome;
		float MinEnergyIncome;
		float MaxEnergyFactor;
		float MaxMetalFactor;
	} StorageConfig;

	vector <int> EnergyMakers;
	vector <int> MetalMakers;
	vector <int> MetalExtracters;
};

class ResourceUnitHandler : public TaskFactory
{
public:
	ResourceUnitHandler(CGlobals *g);

	void UnfinishedUnitDestroyed (aiUnit *unit);
	void UnitDestroyed (aiUnit *unit);
	void UnitFinished (aiUnit *unit);

	aiUnit* CreateUnit (int id, BuildTask *task); // should not register the unit - since it's not finished yet
	BuildTask* GetNewBuildTask ();

	const char *GetName();
	void Update ();

	struct Task : public BuildTask
	{
		Task (const UnitDef *def) : BuildTask(def) {}
		bool InitBuildPos (CGlobals *g);
	};

protected:
	ptrvec <ResourceUnit> units;
	int lastUpdate;
	float lastEnergy;

	friend class TaskManager;

	RUHandlerConfig config;

	BuildTask* CreateStorageTask (UnitDefID id);
	BuildTask* CreateMetalExtractorTask (const UnitDef *def);
};

