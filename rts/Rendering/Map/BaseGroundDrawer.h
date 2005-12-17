// BFGroundTextures.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BASE_GROUND_DRAWER_H__
#define __BASE_GROUND_DRAWER_H__

#include <vector>
#include "Rendering/Env/BaseTreeDrawer.h"
#include "float3.h"

class CBaseGroundDrawer
{
public:
	CBaseGroundDrawer(void);
	virtual ~CBaseGroundDrawer(void);

	virtual void SetExtraTexture(unsigned char* tex,unsigned char* pal,bool highRes)=0;
	virtual void SetHeightTexture()=0;
	virtual void SetMetalTexture(unsigned char* tex,float* extractMap,unsigned char* pal,bool highRes){};
	virtual void SetPathMapTexture(){};
	virtual void ToggleLosTexture()=0;
	virtual void Draw(bool drawWaterReflection=false,bool drawUnitReflection=false)=0;
	virtual bool UpdateTextures()=0;
	virtual void DrawShadowPass(void);

	bool updateFov;
	
	int viewRadius;
	int striptype;
	float baseTreeDistance;

	float infoTexAlpha;

	bool drawLos;
	bool drawExtraTex;
	bool drawMetalMap;
	bool drawPathMap;
	bool drawHeightMap;

	unsigned int infoTex;

	void AddFrustumRestraint(float3 side);
	struct fline {
		float base;
		float dir;
	};
	std::vector<fline> right,left;
protected:
	void UpdateCamRestraints(void);
};

extern CBaseGroundDrawer* groundDrawer;

#endif // __BASE_GROUND_DRAWER__
