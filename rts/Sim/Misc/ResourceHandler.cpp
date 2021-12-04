/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ResourceHandler.h"
#include "ResourceMapAnalyzer.h"
#include "Map/MapInfo.h" // for the metal extractor radius
#include "Map/ReadMap.h" // for the metal map
#include "Map/MetalMap.h"

#include <cfloat>


CR_BIND(CResourceHandler, )
CR_REG_METADATA(CResourceHandler, (
	CR_IGNORED(resourceDescriptions),
	CR_IGNORED(resourceMapAnalyzers),
	CR_MEMBER(metalResourceId),
	CR_MEMBER(energyResourceId),

	CR_POSTLOAD(PostLoad)
))


static CResourceHandler instance;


CResourceHandler* CResourceHandler::GetInstance() { return &instance; }

void CResourceHandler::CreateInstance() { instance.Init(); }
void CResourceHandler::FreeInstance() { instance.Kill(); }


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void CResourceHandler::AddResources() {
	resourceDescriptions.clear();
	resourceDescriptions.reserve(SResourcePack::MAX_RESOURCES);
	resourceMapAnalyzers.clear();
	resourceMapAnalyzers.reserve(SResourcePack::MAX_RESOURCES);

	CResourceDescription rMetal;
	rMetal.name = "Metal";
	rMetal.optimum = FLT_MAX;
	rMetal.extractorRadius = mapInfo->map.extractorRadius;
	rMetal.maxWorth = mapInfo->map.maxMetal;
	metalResourceId = AddResource(rMetal);

	CResourceDescription rEnergy;
	rEnergy.name = "Energy";
	rEnergy.optimum = FLT_MAX;
	rEnergy.extractorRadius = 0.0f;
	rEnergy.maxWorth = 0.0f;
	energyResourceId = AddResource(rEnergy);
}

int CResourceHandler::AddResource(const CResourceDescription& resource)
{
	// GetResourceMapAnalyzer returns a pointer, no resizing allowed
	assert(resourceDescriptions.size() < SResourcePack::MAX_RESOURCES);

	resourceDescriptions.push_back(resource);
	resourceMapAnalyzers.emplace_back(resourceDescriptions.size() - 1);
	return (resourceDescriptions.size() - 1);
}


const CResourceDescription* CResourceHandler::GetResource(int resourceId) const
{
	if (IsValidId(resourceId))
		return &resourceDescriptions[resourceId];

	return nullptr;
}

const CResourceDescription* CResourceHandler::GetResourceByName(const std::string& resourceName) const
{
	return GetResource(GetResourceId(resourceName));
}

int CResourceHandler::GetResourceId(const std::string& resourceName) const
{
	const auto pred = [&](const CResourceDescription& rd) { return (resourceName == rd.name); };
	const auto iter = std::find_if(resourceDescriptions.cbegin(), resourceDescriptions.cend(), pred);
	return ((iter == resourceDescriptions.end())? -1: (iter - resourceDescriptions.cbegin()));
}

const unsigned char* CResourceHandler::GetResourceMap(int resourceId) const
{
	if (resourceId == GetMetalId())
		return (metalMap.GetDistributionMap());

	return nullptr;
}

size_t CResourceHandler::GetResourceMapSize(int resourceId) const
{
	if (resourceId == GetMetalId())
		return (GetResourceMapWidth(resourceId) * GetResourceMapHeight(resourceId));

	return 0;
}

size_t CResourceHandler::GetResourceMapWidth(int resourceId) const
{
	if (resourceId == GetMetalId())
		return mapDims.hmapx;

	return 0;
}

size_t CResourceHandler::GetResourceMapHeight(int resourceId) const
{
	if (resourceId == GetMetalId())
		return mapDims.hmapy;

	return 0;
}

const CResourceMapAnalyzer* CResourceHandler::GetResourceMapAnalyzer(int resourceId)
{
	if (!IsValidId(resourceId))
		return nullptr;

	CResourceMapAnalyzer* rma = &resourceMapAnalyzers[resourceId];

	if (rma->GetNumSpots() < 0)
		rma->Init();

	return rma;
}

