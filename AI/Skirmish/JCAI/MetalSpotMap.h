//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
class InfoMap;

typedef int MetalSpotID;

class aiUnit;

struct MetalSpot
{
	MetalSpot() { metalProduction=height=0.0f; extractDepth=0.0f; unit=0; }

	int2 pos;
	float height;
	float metalProduction;
	float extractDepth;
	int block;

	aiUnit *unit;
};

class MetalSpotMap
{
public:
	MetalSpotMap();
	~MetalSpotMap();

	void Initialize(IAICallback *cb);

	// find a spot with less extraction depth than current
	MetalSpotID FindSpot (const float3& startpos, InfoMap* infoMap, float extractDepth, bool water=false);
	void SetSpotExtractDepth (MetalSpotID spot, float extractDepth);
	MetalSpot* GetSpot(MetalSpotID id) { return &spots[id]; }
	int GetNumSpots();

	bool LoadCache (const char *mapname);
	bool SaveCache (const char *mapname);
	string GetMapCacheName (const char *mapname);

	float GetAverageProduction () { return averageProduction; }
	float GetAverageMetalDensity () { return averageMetal; }

	bool debugShowSpots;

protected:
	struct Block {
		Block(){first=count=taken=0;}
		int first;
		int count;
		int taken;
	};

	Block *blocks;
	int width,height;//blocks width and height

	void CalcBlocks (IAICallback *cb);
	void AlignSpots (const uchar *metalmap, int mapw, int maph);

	std::vector <MetalSpot> spots;
	float averageProduction;
	float averageMetal;
};


