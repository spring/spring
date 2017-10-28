/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FireBallProjectile.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/creg/STL_Deque.h"

CR_BIND_DERIVED(CFireBallProjectile, CWeaponProjectile, )

CR_REG_METADATA(CFireBallProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(sparks)
))


CR_BIND(CFireBallProjectile::Spark, )
CR_REG_METADATA_SUB(CFireBallProjectile, Spark, (
	CR_MEMBER(pos),
	CR_MEMBER(speed),
	CR_MEMBER(size),
	CR_MEMBER(ttl)
))


CFireBallProjectile::CFireBallProjectile(const ProjectileParams& params): CWeaponProjectile(params)
{
	projectileType = WEAPON_FIREBALL_PROJECTILE;

	if (weaponDef != nullptr) {
		SetRadiusAndHeight(weaponDef->collisionSize, 0.0f);
		drawRadius = weaponDef->size;
	}
}

void CFireBallProjectile::Draw(CVertexArray* va)
{
	unsigned char col[4] = { 255, 150, 100, 1 };

	float3 interPos = checkCol ? drawPos : pos;
	float size = radius * 1.3f;

	int numSparks = sparks.size();
	int numFire = std::min(10, numSparks);
	va->EnlargeArrays((numSparks + numFire) * 4, 0, VA_SIZE_TC);

	for (int i = 0; i < numSparks; i++) {
		//! CAUTION: loop count must match EnlargeArrays above
		col[0] = (numSparks - i) * 12;
		col[1] = (numSparks - i) *  6;
		col[2] = (numSparks - i) *  4;

		#define ept projectileDrawer->explotex
		va->AddVertexQTC(sparks[i].pos - camera->GetRight() * sparks[i].size - camera->GetUp() * sparks[i].size, ept->xstart, ept->ystart, col);
		va->AddVertexQTC(sparks[i].pos + camera->GetRight() * sparks[i].size - camera->GetUp() * sparks[i].size, ept->xend,   ept->ystart, col);
		va->AddVertexQTC(sparks[i].pos + camera->GetRight() * sparks[i].size + camera->GetUp() * sparks[i].size, ept->xend,   ept->yend,   col);
		va->AddVertexQTC(sparks[i].pos - camera->GetRight() * sparks[i].size + camera->GetUp() * sparks[i].size, ept->xstart, ept->yend,   col);
		#undef ept
	}

	int maxCol = numFire;
	if (checkCol) {
		maxCol = 10;
	}

	for (int i = 0; i < numFire; i++) //! CAUTION: loop count must match EnlargeArrays above
	{
		col[0] = (maxCol - i) * 25;
		col[1] = (maxCol - i) * 15;
		col[2] = (maxCol - i) * 10;
		#define dgt projectileDrawer->dguntex
		va->AddVertexQTC(interPos - camera->GetRight() * size - camera->GetUp() * size, dgt->xstart, dgt->ystart, col);
		va->AddVertexQTC(interPos + camera->GetRight() * size - camera->GetUp() * size, dgt->xend ,  dgt->ystart, col);
		va->AddVertexQTC(interPos + camera->GetRight() * size + camera->GetUp() * size, dgt->xend ,  dgt->yend,   col);
		va->AddVertexQTC(interPos  -camera->GetRight() * size + camera->GetUp() * size, dgt->xstart, dgt->yend,   col);
		#undef dgt
		interPos = interPos - speed * 0.5f;
	}
}

void CFireBallProjectile::Update()
{
	if (checkCol) {
		if (!luaMoveCtrl) {
			pos += speed;
			speed.y += (mygravity * weaponDef->gravityAffected);
		}

		if (weaponDef->noExplode && TraveledRange())
			checkCol = false;

		EmitSpark();
	} else {
		deleteMe |= sparks.empty();
	}

	for (unsigned int i = 0; i < sparks.size(); i++) {
		if ((--sparks[i].ttl) == 0) {
			sparks.pop_back();
			break;
		}

		sparks[i].pos += (sparks[i].speed * checkCol);
		sparks[i].speed *= 0.95f;
	}

	explGenHandler->GenExplosion(cegID, pos, speed, ttl, !sparks.empty() ? sparks[0].size : 0.0f, 0.0f, NULL, NULL);
	UpdateGroundBounce();
	UpdateInterception();
}

void CFireBallProjectile::EmitSpark()
{
	Spark spark;
	const float x = guRNG.NextFloat() - 0.5f;
	const float y = guRNG.NextFloat() - 0.5f;
	const float z = guRNG.NextFloat() - 0.5f;

	spark.speed = (speed * 0.95f) + float3(x, y, z);
	spark.pos = pos - speed * (guRNG.NextFloat() + 3) + spark.speed * 3;
	spark.size = 5.0f;
	spark.ttl = 15;

	sparks.push_front(spark);
}

void CFireBallProjectile::Collision()
{
	CWeaponProjectile::Collision();
	deleteMe = false;
}

int CFireBallProjectile::GetProjectilesCount() const
{
	int numSparks = sparks.size();
	int numFire = std::min(10, numSparks);
	return (numSparks + numFire);
}
