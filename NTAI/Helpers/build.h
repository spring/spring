#include "aicallback.h"
#include "unitdef.h"

struct SearchOffset {
	int dx,dy;
	int qdist; // dx*dx+dy*dy
};
class BuildPlacer{
public:
	BuildPlacer(Global* GLI);
	~BuildPlacer();
	bool Init();
	bool FindBuildPosition (const UnitDef *def, const float3& startPos, float3& out);
	void Mark (const UnitDef* def, const float3& pos, bool occupy);
	void Calc2DBuildPos (float3& pos, const UnitDef* def);
	bool WriteTGA (const char *fn);
	float3 Pos2BuildPos(float3 pos, const UnitDef* ud);
	float3 ClosestBuildSite(const UnitDef* unitdef,float3 pos,float searchRadius,int minDist);
private:
	Global* G;
	int w,h;
	//uchar *bmap;
	map<int,uchar> bmap;
	int GetUnitSpace (const UnitDef* def);
	inline uchar& Get(int x,int y) { return bmap[w*y+x]; }
};