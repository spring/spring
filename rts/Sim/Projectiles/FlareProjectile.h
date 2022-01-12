/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FLARE_PROJECTILE_H
#define FLARE_PROJECTILE_H

#include <array>
#include "Projectile.h"

class CFlareProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CFlareProjectile)

private:
	CFlareProjectile() {}

public:
	CFlareProjectile(const float3& pos, const float3& speed, CUnit* owner, int activateFrame);

	void Update() override;
	void Draw(GL::RenderDataBufferTC* va) const override;

	int GetProjectilesCount() const override { return (subProjPos.size()); }

public:
	int activateFrame;
	int deathFrame;

	unsigned int numSubProjs;
	unsigned int maxSubProjs;
	unsigned int lastSubProj;

	// position and velocity for each sub-salvo particle
	std::array<float3, 16> subProjPos;
	std::array<float3, 16> subProjVel;

	float alphaFalloff;
};

#endif // FLARE_PROJECTILE_H
