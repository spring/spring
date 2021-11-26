/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXP_GEN_SPAWNER_H
#define EXP_GEN_SPAWNER_H

#include "Projectile.h"

class IExplosionGenerator;

//! spawns a given explosion-generator after \<delay\> frames
class CExpGenSpawner : public CProjectile
{
	CR_DECLARE_DERIVED(CExpGenSpawner)
public:
	CExpGenSpawner();
	~CExpGenSpawner() {}
	void Serialize(creg::ISerializer* s);

	void Update() override;

	int GetProjectilesCount() const override { return 0; }

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	int delay = 1;
	float damage = 0.0f;

	IExplosionGenerator* explosionGenerator = nullptr;
};

#endif // EXP_GEN_SPAWNER_H
