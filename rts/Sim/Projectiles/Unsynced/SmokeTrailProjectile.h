#ifndef SMOKETRAILPROJECTILE_H
#define SMOKETRAILPROJECTILE_H
// SmokeTrailProjectile.h: interface for the CSmokeTrailProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "Sim/Projectiles/Projectile.h"

struct AtlasedTexture;

class CSmokeTrailProjectile : public CProjectile
{
	CR_DECLARE(CSmokeTrailProjectile);
public:
	void Update();
	void Draw();
	CSmokeTrailProjectile(const float3& pos1,const float3& pos2,const float3& dir1,const float3& dir2, CUnit* owner,bool firstSegment,bool lastSegment,float size=1,float time=80,float color=0.7f,bool drawTrail=true,CProjectile* drawCallback=0,AtlasedTexture* texture=0 GML_PARG_H);
	virtual ~CSmokeTrailProjectile();

	float3 pos1;
	float3 pos2;
	float orgSize;
	int creationTime;
	int lifeTime;
	float color;
	float3 dir1;
	float3 dir2;
	bool drawTrail;

	float3 dirpos1;
	float3 dirpos2;
	float3 midpos;
	float3 middir;
	bool drawSegmented;
	bool firstSegment,lastSegment;
	CProjectile* drawCallbacker;
	AtlasedTexture* texture;
};

#endif /* SMOKETRAILPROJECTILE_H */
