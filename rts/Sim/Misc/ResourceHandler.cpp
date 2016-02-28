/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ResourceHandler.h"

#include "ResourceMapAnalyzer.h"
#include "Map/MapInfo.h" // for the metal extractor radius
#include "Map/ReadMap.h" // for the metal map
#include "Map/MetalMap.h"
#include "GlobalSynced.h" // for the map size
#include <float.h>


CR_BIND(CResourceHandler, )
CR_REG_METADATA(CResourceHandler, (
	CR_MEMBER(resources),
	CR_IGNORED(resourceMapAnalyzers),
	CR_MEMBER(metalResourceId),
	CR_MEMBER(energyResourceId),

	CR_POSTLOAD(PostLoad)
))


CResourceHandler* CResourceHandler::instance = NULL;

CResourceHandler* CResourceHandler::GetInstance() {
	assert(instance != NULL);
	return instance;
}

void CResourceHandler::CreateInstance()
{
	if (instance == NULL) {
		instance = new CResourceHandler();
	}
}
void CResourceHandler::FreeInstance()
{
	delete instance;
	instance = NULL;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CResourceHandler::CResourceHandler()
{
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
	resources.push_back(resource);
	resourceMapAnalyzers.push_back(nullptr);
	return resources.size() - 1;
}

void CResourceHandler::PostLoad()
{
	resourceMapAnalyzers.resize(resources.size(), nullptr);
}

const CResourceDescription* CResourceHandler::GetResource(int resourceId) const
{
	if (IsValidId(resourceId))
		return &resources[resourceId];

	return NULL;
}

const CResourceDescription* CResourceHandler::GetResourceByName(const std::string& resourceName) const
{
	return GetResource(GetResourceId(resourceName));
}

int CResourceHandler::GetResourceId(const std::string& resourceName) const
{
	for (size_t r = 0; r < resources.size(); ++r) {
		if (resources[r].name == resourceName) {
			return r;
		}
	}

	return -1;
}

const unsigned char* CResourceHandler::GetResourceMap(int resourceId) const {
	if (resourceId == GetMetalId())
		return (readMap->metalMap->GetDistributionMap());

	return NULL;
}

size_t CResourceHandler::GetResourceMapSize(int resourceId) const {
	if (resourceId == GetMetalId())
		return (GetResourceMapWidth(resourceId) * GetResourceMapHeight(resourceId));

	return 0;
}

size_t CResourceHandler::GetResourceMapWidth(int resourceId) const {
	if (resourceId == GetMetalId())
		return mapDims.hmapx;

	return 0;
}

size_t CResourceHandler::GetResourceMapHeight(int resourceId) const {
	if (resourceId == GetMetalId())
		return mapDims.hmapy;

	return 0;
}

const CResourceMapAnalyzer* CResourceHandler::GetResourceMapAnalyzer(int resourceId) {

	if (!IsValidId(resourceId))
		return NULL;

	CResourceMapAnalyzer*& rma = resourceMapAnalyzers[resourceId];

	if (rma == nullptr)
		rma = new CResourceMapAnalyzer(resourceId);

	return rma;
}



bool CResourceHandler::IsValidId(int resourceId) const {
	return (resourceId >= 0) && (static_cast<size_t>(resourceId) < resources.size());
}

