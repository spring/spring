#include "StdAfx.h"
#include "ExpGenSpawner.h"
#include "ExplosionGenerator.h"

CR_BIND_DERIVED(CExpGenSpawner, CProjectile, );

CR_REG_METADATA(CExpGenSpawner, 
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(delay),
		CR_MEMBER(dir),
		CR_MEMBER(damage),
		CR_MEMBER(explosionGenerator),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_RESERVED(8)
));

CExpGenSpawner::CExpGenSpawner(void) :
	CProjectile(),
	damage(0)
{
	checkCol=false;
	deleteMe=false;
	synced=false;
}

CExpGenSpawner::~CExpGenSpawner(void)
{
}

void CExpGenSpawner::Update()
{
	if(!delay--)
	{
		explosionGenerator->Explosion(pos, damage, 0, owner(), 0, NULL, dir);
		deleteMe=true;
	}
}
