/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TORPEDO_PROJECTILE_H
#define TORPEDO_PROJECTILE_H

#include "WeaponProjectile.h"

class CTorpedoProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CTorpedoProjectile)
public:
	// creg only
	CTorpedoProjectile() { }
	CTorpedoProjectile(const ProjectileParams& params);

	void Update() override;
	void Draw(GL::RenderDataBufferTC* va) const override;

	int GetProjectilesCount() const override { return 8; }

	void SetIgnoreError(bool b) { ignoreError = b; }

private:
	float3 UpdateTargetingPos();
	float3 UpdateTargetingDir(const float3& targetObjVel);

private:
	bool ignoreError;

	float tracking;
	float maxSpeed;
	int nextBubble;

	float texx;
	float texy;
};


#endif /* TORPEDO_PROJECTILE_H */
