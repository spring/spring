#ifndef BEAMLASERPROJECTILE_H
#define BEAMLASERPROJECTILE_H

#include "Projectile.h"

class CBeamLaserProjectile :
	public CProjectile
{
public:
	CBeamLaserProjectile(const float3& startPos,const float3& endPos,float endAlpha,const float3& color,CUnit* owner,float thickness);
	~CBeamLaserProjectile(void);

	float3 startPos;
	float3 endPos;
	float3 color;
	float endAlpha;
	float thickness;

	void Update(void);
	void Draw(void);
};

#endif
