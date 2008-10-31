#ifndef FLAREPROJECTILE_H
#define FLAREPROJECTILE_H

#include "Projectile.h"
#include <vector>

class CFlareProjectile :
	public CProjectile
{
public:
	CR_DECLARE(CFlareProjectile);
	CFlareProjectile(const float3& pos,const float3& speed,CUnit* owner,int activateFrame GML_PARG_H);
	~CFlareProjectile();
	void Update(void);
	void Draw(void);

	int activateFrame;
	int deathFrame;

	int numSub;
	int lastSub;
	std::vector<float3> subPos;
	std::vector<float3> subSpeed;
	float alphaFalloff;
};

#endif
