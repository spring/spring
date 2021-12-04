// _____________________________________________________
//
// RAI - Skirmish AI for Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAIG_RESOURCE_MAP_H
#define RAIG_RESOURCE_MAP_H

#include "LegacyCpp/IAICallback.h"
#include "GTerrainMap.h"
#include <set>
#include <string>
using std::map;
using std::set;
using std::vector;

struct ResourceSiteDistance
{
	ResourceSiteDistance(float minimalDistance)
	{
		minDistance = minimalDistance;
		bestDistance = 0;
		bestPathType = -1;
	};
	float minDistance;
	int bestPathType; // set only once, the first time a pathType distance is found successfully
	float *bestDistance; // set only once (= distance->second), the first time a pathType distance is found
	map<int,float> distance; // key = pathType
	vector<float3> pathDebug; // Only used for temporary storage & debugging
};

struct ResourceSite
{
	ResourceSite(float3& rsPosition, int rsFeatureID=-1, const FeatureDef* fd=0);
	float GetResourceDistance(ResourceSite* RS, const int& pathType);

	int type;		// 0=metal/Non-Feature, 1=geo/Feature
	float amount;	// For metal-sites (this * ud->extractsMetal = the predicted income)
	int featureID;	// Valid if it's a Geo-Site, otherwise -1
	const FeatureDef *featureD;	// Valid if 'featureID' is set
	float3 position;
	set<int> options; // key = ud->id, a list of posible units to be built at the site
	map<ResourceSite*,ResourceSiteDistance> siteDistance;
};

class GlobalResourceMap
{
public:
	GlobalResourceMap(IAICallback* cb, cLogFile* logfile, GlobalTerrainMap* TM);
	~GlobalResourceMap();
	float3 GetMetalMapPosition(const float3& position) const;

	ResourceSite** R[2];	// a list of each resource type
	int RSize[2];			// # of each resource type
	float averageMetalSite; // (this * ud->extractsMetal = the predicted income)
	bool isMetalMap;		// if true then RSize[0] will = 0, use UNFINISHED

private:
	struct MetalMapSector
	{
		MetalMapSector()
		{
			isMetalSector = false;
			closestMetalSector = 0;
			percentMetal = 0.0;
			S = NULL;
//			totalMetal = 0.0;
		};

		bool isMetalSector;	// There is enough metal in and around the sector that metal extractors can be built randomly
		MetalMapSector* closestMetalSector; // not initialized, the closest sector with isMetalSector
		TerrainMapSector* S;

		// only used during initialization
		float percentMetal; // 0-100
//		float totalMetal;
	};
	MetalMapSector *sector;

	// only used during initialization
	struct sMetalMapSquare
	{
		bool assessing;		//
		float metal;		// unused metal at this square
		float totalMetal;	// (this * ud->extractsMetal = the predicted income)
		float inaccuracy;	// a high value means the totalMetal is not near the center of the x,z position
		int x;
		int z;
	};
	struct sMMRadiusSquare
	{
		bool inRange;		// true if it is within the extractor radius, a little faster for calculations
		float distance;		// the distance between this square and the center
	};
	void SetLimitBoundary(int &xMin, int &xMax, int &xMMRS, int &zMin, int &zMax, int &zMMRS, const int &increment);
	void SetLimitBoundary(int &xMin, int &xMax, const int &xIncrement, int &zMin, int &zMax, const int &zIncrement);
	void FindMMSTotalMetal(const int &xMMin, const int &xMMax, const int &zMMin, const int &zMMax);
	void FindMMSInaccuracy(const int &xM, const int &zM);

	int *edgeOffset;		// Same dimension size as MMRS
	sMMRadiusSquare **MMRS;	// 2 Dimensionial Array [ExtractorDiameter][ExtractorDiameter]
	sMetalMapSquare **MMS;	// 2 Dimensionial Array [GetMapWidth()/2][GetMapHeight()/2]
	sMetalMapSquare **MMSAssessing;	// 1 Dimensionial Array of pointers
	int MMSAssessingSize;
	int MMZSize;
	int MMXSize;
	int MMExtractorRadiusI;

	string resourceFileName_w;

	// needed to save the file
	string relResourceFileName;
	bool saveResourceFile;
	vector<int> saveUD;
	vector<int> saveF;
	int saveSectorSize;

	cLogFile* l;
};

#endif
