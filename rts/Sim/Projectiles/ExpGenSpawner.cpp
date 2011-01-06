/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "ExpGenSpawner.h"
#include "ExplosionGenerator.h"

CR_BIND_DERIVED(CExpGenSpawner, CProjectile, );

CR_REG_METADATA(CExpGenSpawner,
(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(delay),
		CR_MEMBER(dir),
		CR_MEMBER(damage),
		CR_MEMBER(explosionGenerator),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_RESERVED(8)
));

CExpGenSpawner::CExpGenSpawner() :
	CProjectile(),
	delay(1),
	damage(0.0f),
	explosionGenerator(NULL)
{
	checkCol = false;
	deleteMe = false;
	synced = true;
}

void CExpGenSpawner::Update()
{
	if (delay-- <= 0) {
		explosionGenerator->Explosion(0, pos, damage, 0, owner(), 0, NULL, dir);
		deleteMe = true;
	}
}
