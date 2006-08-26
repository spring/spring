// BFGroundDrawer.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BF_GROUND_DRAWER_H__
#define __BF_GROUND_DRAWER_H__

#include "Map/BaseGroundDrawer.h"

class CVertexArray;
class CSmfReadMap;
class CBFGroundTextures;


/**
Map drawer implementation for the CSmfReadMap map system.
*/
class CBFGroundDrawer :
	public CBaseGroundDrawer
{
public:
	CBFGroundDrawer(CSmfReadMap *rm);
	~CBFGroundDrawer(void);
	void Draw(bool drawWaterReflection=false,bool drawUnitReflection=false,unsigned int overrideVP=0);

	void IncreaseDetail();
	void DecreaseDetail();

protected:
	int viewRadius;
	CSmfReadMap *map;
	CBFGroundTextures *textures;

	int numBigTexX;
	int numBigTexY;

	float* heightData;
	int heightDataX;

	CVertexArray* va;

	struct fline {
		float base;
		float dir;
	};
	std::vector<fline> right,left;

	friend class CSmfReadMap;

	unsigned int groundVP;
	unsigned int groundShadowVP;
	unsigned int groundFPShadow;

	inline void DrawVertexA(int x,int y);
	inline void DrawVertexA(int x,int y,float height);
	inline void EndStrip();
	void DrawGroundVertexArray();
	void SetupTextureUnits(bool drawReflection,unsigned int overrideVP);
	void ResetTextureUnits(bool drawReflection,unsigned int overrideVP);

	void AddFrustumRestraint(float3 side);
	void UpdateCamRestraints();
	void Update(){}
public:
	void DrawShadowPass();
};

#endif // __BF_GROUND_DRAWER_H__
