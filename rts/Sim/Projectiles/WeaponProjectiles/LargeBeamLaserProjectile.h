/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LARGE_BEAM_LASER_PROJECTILE_H
#define LARGE_BEAM_LASER_PROJECTILE_H

#include "WeaponProjectile.h"
#include "Rendering/Textures/TextureAtlas.h"

class CLargeBeamLaserProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CLargeBeamLaserProjectile)
public:
	// creg only
	CLargeBeamLaserProjectile() { }

	CLargeBeamLaserProjectile(const ProjectileParams& params);

	void Update() override;
	void Draw(GL::RenderDataBufferTC* va) const override;
	void DrawOnMinimap(GL::RenderDataBufferC* va) override;

	int GetProjectilesCount() const override { return 32; } // approx

private:
	unsigned char coreColStart[4];
	unsigned char edgeColStart[4];

	float thickness;
	float corethickness;
	float flaresize;
	float tilelength;
	float scrollspeed;
	float pulseSpeed;
	float decay;

	AtlasedTexture beamtex;
	AtlasedTexture sidetex;
};

#endif // LARGE_BEAM_LASER_PROJECTILE_H
