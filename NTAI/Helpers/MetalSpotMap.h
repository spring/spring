#include <vector>
#include <set>
#include <list>

#include "../rts/float3.h"
#include "../rts/iaicallback.h"
/*struct MetalSpotInfo
{
	MetalSpotInfo() { metalvalue=0; taken=false; }

	int2 spotpos;
	uchar metalvalue;
	//float height, heightDif;
	float metalProduction;
	bool taken;
};
*/
struct int2 {
	int2(){};
	int2(int x,int y):x(x),y(y){};
	int x;
	int y;
};
struct float2 {
	float x;
	float y;
};
class MetalSpotMap
{
public:
	MetalSpotMap();
	~MetalSpotMap();
	//bool metalcmp(float3 a, float3 b);
	float3 MPos2BuildPos(const UnitDef* ud, float3 pos);
	void Initialize(IAICallback *cb);
	//MetalSpotInfo* Get (int x,int y) { return &spotmap[y*w+x]; }
	int2 GetFrom3D (const float3& pos);
	// find a spot to build a mex on, and marks it "taken"
	//int2 GetEmptySpot (const float3& startpos, InfoMap* infoMap, bool water=false); 
	// unmarks the spot
	//void MarkSpot (int2 pos, bool mark);

	//bool LoadCache (const char *mapname);
	//bool SaveCache (const char *mapname);
	//string GetMapCacheName (const char *mapname);

	//float GetAverageProduction () { return averageProduction; }
	//float GetAverageMetalDensity () { return averageMetal; }

	//bool debugShowSpots;
	std::list<float3>	metalpatch;
	//std::vector<MetalSpotInfo> hotspots;



protected:
	int metalblockw, blockw;
	int w,h;
	//MetalSpotInfo* spotmap;
	float averageProduction;
	float averageMetal;
	unsigned char *freeSquare;
};


