/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FLARE_PROJECTILE_H
#define FLARE_PROJECTILE_H

#include "Projectile.h"
#include <vector>

class CFlareProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CFlareProjectile)

private:
	CFlareProjectile() {}

public:
	CFlareProjectile(const float3& pos, const float3& speed, CUnit* owner, int activateFrame);
	~CFlareProjectile();

	void Update() override;
	void Draw(GL::RenderDataBufferTC* va) const override;

	int GetProjectilesCount() const override { return (subPos.size()); }

public:
	int activateFrame;
	int deathFrame;

	int numSub;
	int lastSub;

	std::vector<float3> subPos;
	std::vector<float3> subSpeed;

	float alphaFalloff;
};

#endif // FLARE_PROJECTILE_H
