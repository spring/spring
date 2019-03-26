/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BEAM_LASER_PROJECTILE_H
#define BEAM_LASER_PROJECTILE_H

#include "WeaponProjectile.h"

class CBeamLaserProjectile: public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CBeamLaserProjectile)
public:
	// creg only
	CBeamLaserProjectile() { }

	CBeamLaserProjectile(const ProjectileParams& params);

	void Update() override;
	void Draw(GL::RenderDataBufferTC* va) const override;
	void DrawOnMinimap(GL::RenderDataBufferC* va) override;

	int GetProjectilesCount() const override { return 8; }

private:
	unsigned char coreColStart[4];
	unsigned char coreColEnd[4];
	unsigned char edgeColStart[4];
	unsigned char edgeColEnd[4];

	float thickness;
	float corethickness;
	float flaresize;
	float decay;
	float midtexx;
};

#endif // BEAM_LASER_PROJECTILE_H
