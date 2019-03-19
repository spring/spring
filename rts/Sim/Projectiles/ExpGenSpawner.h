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

	virtual void Update() override;

	virtual int GetProjectilesCount() const override;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	int delay;
	float damage;

	IExplosionGenerator* explosionGenerator;
};

#endif // EXP_GEN_SPAWNER_H
