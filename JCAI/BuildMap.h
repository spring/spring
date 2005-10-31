//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#ifndef JC_BUILDMAP_H
#define JC_BUILDMAP_H

class MetalSpotMap;

// Chooses build positions - idea is based on the MapGridUtils class from Alik
class BuildMap
{
public:
	BuildMap();
	~BuildMap();

	bool Init (IAICallback *cb, MetalSpotMap *msm);
	bool FindBuildPosition (const UnitDef *def, const float3& startPos, float3& out);
	void Mark (const UnitDef* def, const float3& pos, bool occupy);
	void Calc2DBuildPos (float3& pos, const UnitDef* def);
	bool WriteTGA (const char *fn);

protected:
	int w,h;
	uchar *map;
	MetalSpotMap* metalmap;
	IAICallback *callback;

	int GetUnitSpace (const UnitDef* def);

	inline uchar& Get(int x,int y) { return map[w*y+x]; }
};


#endif
