/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ResourceHandler.h"

#include "ResourceMapAnalyzer.h"
#include "Map/MapInfo.h" // for the metal extractor radius
#include "Map/ReadMap.h" // for the metal map
#include "Map/MetalMap.h"
#include "GlobalSynced.h" // for the map size

CR_BIND(CResourceHandler, );

CR_REG_METADATA(CResourceHandler, (
	CR_MEMBER(resources),
//	CR_MEMBER(resourceMapAnalyzers),
	CR_MEMBER(metalResourceId),
	CR_MEMBER(energyResourceId)
));

CResourceHandler* CResourceHandler::instance;

CResourceHandler* CResourceHandler::GetInstance()
{
	if (instance == NULL) {
		instance = new CResourceHandler();
	}
	return instance;
}
void CResourceHandler::FreeInstance()
{
	delete instance;
	instance = NULL;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// This is the minimum of a max float for all platforms
// see: http://en.wikipedia.org/wiki/Float.h
static const float MAX_FLOAT = 1E+37;

CResourceHandler::CResourceHandler()
{
	CResource rMetal;
	rMetal.name = "Metal";
	rMetal.optimum = MAX_FLOAT;
	rMetal.extractorRadius = mapInfo->map.extractorRadius;
	rMetal.maxWorth = mapInfo->map.maxMetal;
	metalResourceId = AddResource(rMetal);

	CResource rEnergy;
	rEnergy.name = "Energy";
	rEnergy.optimum = MAX_FLOAT;
	rEnergy.extractorRadius = 0.0f;
	rEnergy.maxWorth = 0.0f;
	energyResourceId = AddResource(rEnergy);
}

CResourceHandler::~CResourceHandler()
{
}

int CResourceHandler::AddResource(const CResource& resource) {

	resources.push_back(resource);
	int resourceId = resources.size()-1;
	resourceMapAnalyzers[resourceId] = NULL;
	return resourceId;
}

const CResource* CResourceHandler::GetResource(int resourceId) const
{
	if (IsValidId(resourceId)) {
		return &resources[resourceId];
	} else {
		return NULL;
	}
}

const CResource* CResourceHandler::GetResourceByName(const std::string& resourceName) const
{
	return GetResource(GetResourceId(resourceName));
}

int CResourceHandler::GetResourceId(const std::string& resourceName) const
{
	for (size_t r=0; r < resources.size(); ++r) {
		if (resources[r].name == resourceName) {
			return (int)r;
		}
	}
	return -1;
}

const unsigned char* CResourceHandler::GetResourceMap(int resourceId) const {

	if (resourceId == GetMetalId()) {
		return &readmap->metalMap->metalMap[0];
	} else {
		return NULL;
	}
}
size_t CResourceHandler::GetResourceMapSize(int resourceId) const {

	if (resourceId == GetMetalId()) {
		return GetResourceMapWidth(resourceId) * GetResourceMapHeight(resourceId);
	} else {
		return 0;
	}
}
size_t CResourceHandler::GetResourceMapWidth(int resourceId) const {

	if (resourceId == GetMetalId()) {
		return gs->hmapx;
	} else {
		return 0;
	}
}
size_t CResourceHandler::GetResourceMapHeight(int resourceId) const {

	if (resourceId == GetMetalId()) {
		return gs->hmapy;
	} else {
		return 0;
	}
}
const CResourceMapAnalyzer* CResourceHandler::GetResourceMapAnalyzer(int resourceId) {

	if (!IsValidId(resourceId)) {
		return NULL;
	}

	CResourceMapAnalyzer* rma = resourceMapAnalyzers[resourceId];

	if (rma == NULL) {
		rma = new CResourceMapAnalyzer(resourceId);
		resourceMapAnalyzers[resourceId] = rma;
	}

	return rma;
}


size_t CResourceHandler::GetNumResources() const
{
	return resources.size();
}


//bool CResourceHandler::IsMetal(int resourceId) const
//{
//	const CResource* res = GetResource(resourceId);
//	return res && (res->name == "Metal");
//}
//
//bool CResourceHandler::IsEnergy(int resourceId) const
//{
//	const CResource* res = GetResource(resourceId);
//	return res && (res->name == "Energy");
//}

int CResourceHandler::GetMetalId() const
{
	return metalResourceId;
}

int CResourceHandler::GetEnergyId() const
{
	return energyResourceId;
}

bool CResourceHandler::IsValidId(int resourceId) const {
	return (resourceId >= 0) && (static_cast<size_t>(resourceId) < resources.size());
}
