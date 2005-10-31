#include "BaseAIDef.h"
#include "BaseAIObjects.h"
#include "Tasks.h"
#include "CfgParser.h"
#include "TaskManager.h"
#include "AI_Config.h"
#include "DebugWindow.h"

// half of the resources must be avaiable before it starts building
const float RequiredAllocation=0.2f;

const char *ResourceManager::handlerStr[] = { "Resource", "Force", "Support", "Recon" };

bool ResourceManager::Config::Load (CfgList *sidecfg)
{
	assert (NUM_TASK_TYPES == 4);
	float sum = 0.0f;
	CfgList* cfg = modConfig.root;
	CfgList *weights = dynamic_cast<CfgList*>(cfg->GetValue ("BuildWeights"));
	CfgList *maxtasks = dynamic_cast<CfgList*>(cfg->GetValue ("MaxTasks"));
	for (int a=0;a<NUM_TASK_TYPES;a++)
	{
		BuildWeights[a] = weights ? weights->GetNumeric (handlerStr[a], 1.0f) : 1.0f;
		MaxTasks[a] = maxtasks ? maxtasks->GetInt (handlerStr[a], 1) : 1;
		sum += BuildWeights[a];
	}
	for (int a=0;a<NUM_TASK_TYPES;a++) 
		BuildWeights[a] /= sum;

	logPrintf ("Building weights: Resources=%f, Forces=%f, Defenses=%f\n", 
		BuildWeights [0], BuildWeights[1], BuildWeights [2]);

	return true;
}

ResourceManager::ResourceManager (CGlobals *g) : globals(g)
{
	if (!config.Load (g->sidecfg))
		throw "Failed to load resource manager configuration.";
}


void ResourceManager::ShowDebugInfo(DbgWindow *wnd)
{
	DbgWndPrintf (wnd, 0, 20, "Build power: %4.1f / %4.1f, %4.1f / %4.1f", 
		totalBuildPower.metal, averageProd.metal, totalBuildPower.energy, averageProd.energy);
	DbgWndPrintf (wnd, 0, 40, "Build multiplier: %2.2f / %2.2f", 
		buildMultiplier.metal, buildMultiplier.energy);

	for (int i=0;i<NUM_TASK_TYPES;i++)
		DbgWndPrintf (wnd, 0, (i + 3) * 23, "%s: resources: %5.1f, weight: %1.3f", globals->taskFactories [i]->GetName(), taskResources[i], config.BuildWeights[i]);
}



void ResourceManager::DistributeResources()
{
	// distribute start resources
	ResourceInfo rs(globals->cb->GetEnergy(), globals->cb->GetMetal());
	float v = ResourceValue (rs);

	for (int a=0;a<NUM_TASK_TYPES;a++)
		taskResources[a] = v * config.BuildWeights [a];
}


bool ResourceManager::AllocateForTask (float energyCost, float metalCost, int resourceType)
{
	assert (resourceType >= 0);

	bool allocated=false;
	float required = ResourceValue (energyCost, metalCost);
	float& reserve = taskResources [resourceType];

	if (reserve + required * (1.0f-RequiredAllocation) >= required) {
		reserve -= required;
		return true;
	}
	return false;
}


void ResourceManager::TakeResources(float r)
{
	for (int a=0;a<NUM_TASK_TYPES;a++)
		taskResources[a] -= r * config.BuildWeights[a];
}


void ResourceManager::Update ()
{
	IAICallback *cb = globals->cb;
	totalBuildPower=ResourceInfo();

	// update average resource production
	const float weight = 0.005;
	ResourceInfo curIncome (cb->GetEnergyIncome (), cb->GetMetalIncome ());
	averageProd = averageProd * (1.0f - weight) + curIncome * weight;

	// update average resource usage
	const float weightU = 0.02;
	ResourceInfo curUsage = ResourceInfo(cb->GetEnergyUsage (), cb->GetMetalUsage ());
	averageUse = averageUse * (1.0f - weightU) + curUsage * weightU;

	// update per-type resource reserves
	float income = ResourceValue (curIncome)/30.0f;
	for (int a=0;a<NUM_TASK_TYPES;a++)
		taskResources [a] += income * config.BuildWeights [a];
}

void ResourceManager::UpdateBuildMultiplier()
{
	if (totalBuildPower.metal > 0.0f && totalBuildPower.energy > 0.0f)
		buildMultiplier = averageProd / totalBuildPower;
	else
		buildMultiplier = ResourceInfo();
}

void ResourceManager::SpillResources (const ResourceInfo& res, int type)
{
	float totalIncome = ResourceValue (res);
	float spill = totalIncome * config.BuildWeights [type];

	float totalw = 0.0f;
	for (int a=0;a<NUM_TASK_TYPES;a++) {
		if (a != type) totalw += config.BuildWeights [a];
	}

	for (int a=0;a<NUM_TASK_TYPES;a++) {
		if (a != type) taskResources [a] += spill * config.BuildWeights[a] / totalw;
	}
	taskResources [type] -= spill;
}
