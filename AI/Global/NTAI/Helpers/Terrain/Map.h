// Map
#include "AICallback.h"
#include "Sim/Units/UnitDef.h"

struct SearchOffset {
	int dx,dy;
	int qdist; // dx*dx+dy*dy
};
struct CSector;
class CMap{
public:
	CMap(Global* GL);
	void Init(){}
	bool LoadTerrainTypes(){
		return false;
	}
	void SaveTerrainTypes(){}
	void GenerateTerrainTypes();
	float3 basepos; // the first base pos (defaulted to by standard)
	vector<float3> base_positions; // base positions (factorys etc)
	float3 nbasepos(float3 pos);// returns the base position nearest to the given float3
	float3 Nearest(float3 pos, vector<float3> locations){return UpVector;}
	float3 distfrom(float3 Start, float3 Target, float distance);
	bool Overlap(float x1, float y1, float w1, float h1,	float x2, float y2, float w2, float h2);
	float3 Rotate(float3 pos, float angle, float3 origin); // rotates a position around a given origin and angle
	
	vector<SearchOffset> GetSearchOffsetTable (int radius);
	float3 ClosestBuildSite(const UnitDef* unitdef,float3 pos,float searchRadius,int minDist);

	bool CheckFloat3(float3 pos);
	t_direction WhichCorner(float3 pos);//returns a value signifying which corner of the map this location is in
	vector<float3> GetSurroundings(float3 pos);//returns the surrounding grid squares or locations of a co-ordinate
	float3 WhichSector(float3 pos);//converts a normal co-ordinate into a Sector co-ordinate on the threat matrix

	float3 Pos2BuildPos(float3 pos, const UnitDef* ud);
	float GetBuildHeight(float3 pos, const UnitDef* unitdef);
	map<int, map<int,CSector> > buildmap;
	int xMapSize;
	int yMapSize;
private:
	Global* G;
};
