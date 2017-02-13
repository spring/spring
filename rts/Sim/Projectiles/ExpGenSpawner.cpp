/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ExpGenSpawner.h"

#include "ExpGenSpawnableMemberInfo.h"
#include "ExplosionGenerator.h"

CR_BIND_DERIVED(CExpGenSpawner, CProjectile, )
CR_REG_METADATA(CExpGenSpawner,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(delay),
		CR_MEMBER(damage),
		CR_MEMBER(explosionGenerator),
	CR_MEMBER_ENDFLAG(CM_Config)
))


CExpGenSpawner::CExpGenSpawner() :
	CProjectile(),
	delay(1),
	damage(0.0f),
	explosionGenerator(NULL)
{
	checkCol = false;
	deleteMe = false;
}


void CExpGenSpawner::Update()
{
	if ((deleteMe |= ((delay--) <= 0))) {
		explosionGenerator->Explosion(pos, dir, damage, 0.0f, 0.0f, owner(), NULL);
	}
}


int CExpGenSpawner::GetProjectilesCount() const
{
	return 0;
}


bool CExpGenSpawner::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_INT  (CExpGenSpawner, delay )
	CHECK_MEMBER_INFO_FLOAT(CExpGenSpawner, damage)
	CHECK_MEMBER_INFO_PTR  (CExpGenSpawner, explosionGenerator, explGenHandler->LoadGenerator)

	return false;
}
