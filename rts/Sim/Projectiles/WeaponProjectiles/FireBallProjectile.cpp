/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "FireBallProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/creg/STL_Deque.h"

CR_BIND_DERIVED(CFireBallProjectile, CWeaponProjectile, (ZeroVector, ZeroVector, NULL, NULL, ZeroVector, NULL));
CR_BIND(CFireBallProjectile::Spark, );

CR_REG_METADATA(CFireBallProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(sparks),
	CR_RESERVED(8)
	));

CR_REG_METADATA_SUB(CFireBallProjectile,Spark,(
	CR_MEMBER(pos),
	CR_MEMBER(speed),
	CR_MEMBER(size),
	CR_MEMBER(ttl),
	CR_RESERVED(8)
	));

CFireBallProjectile::CFireBallProjectile(
	const float3& pos, const float3& speed,
	CUnit* owner, CUnit* target,
	const float3& targetPos,
	const WeaponDef* weaponDef):
	CWeaponProjectile(pos, speed, owner, target, targetPos, weaponDef, NULL, 1)
{
	projectileType = WEAPON_FIREBALL_PROJECTILE;

	if (weaponDef) {
		SetRadius(weaponDef->collisionSize);
		drawRadius = weaponDef->size;
	}

	cegID = gCEG->Load(explGenHandler, cegTag);
}

CFireBallProjectile::~CFireBallProjectile()
{
}

void CFireBallProjectile::Draw()
{
	inArray = true;
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
		va->AddVertexQTC(sparks[i].pos - camera->right * sparks[i].size - camera->up * sparks[i].size, ept->xstart, ept->ystart, col);
		va->AddVertexQTC(sparks[i].pos + camera->right * sparks[i].size - camera->up * sparks[i].size, ept->xend,   ept->ystart, col);
		va->AddVertexQTC(sparks[i].pos + camera->right * sparks[i].size + camera->up * sparks[i].size, ept->xend,   ept->yend,   col);
		va->AddVertexQTC(sparks[i].pos - camera->right * sparks[i].size + camera->up * sparks[i].size, ept->xstart, ept->yend,   col);
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
		va->AddVertexQTC(interPos - camera->right * size - camera->up * size, dgt->xstart, dgt->ystart, col);
		va->AddVertexQTC(interPos + camera->right * size - camera->up * size, dgt->xend ,  dgt->ystart, col);
		va->AddVertexQTC(interPos + camera->right * size + camera->up * size, dgt->xend ,  dgt->yend,   col);
		va->AddVertexQTC(interPos  -camera->right * size + camera->up * size, dgt->xstart, dgt->yend,   col);
		#undef dgt
		interPos = interPos - speed * 0.5f;
	}
}

void CFireBallProjectile::Update()
{
	if (checkCol) {
		if (!luaMoveCtrl) {
			pos += speed;

			if (weaponDef->gravityAffected) {
				speed.y += mygravity;
			}
		}

		if (weaponDef->noExplode) {
			if (TraveledRange())
				checkCol = false;
		}

		EmitSpark();
	} else {
		if (sparks.empty()) {
			deleteMe = true;
		}
	}

	for (unsigned int i = 0; i < sparks.size(); i++) {
		sparks[i].ttl--;
		if (sparks[i].ttl == 0) {
			sparks.pop_back();
			break;
		}
		if (checkCol) {
			sparks[i].pos += sparks[i].speed;
		}
		sparks[i].speed *= 0.95f;
	}

	gCEG->Explosion(cegID, pos, ttl, (sparks.size() > 0) ? sparks[0].size : 0.0f, NULL, 0.0f, NULL, speed);
	UpdateGroundBounce();
}

void CFireBallProjectile::EmitSpark()
{
	Spark spark;
	const float x = (rand() / (float) RAND_MAX) - 0.5f;
	const float y = (rand() / (float) RAND_MAX) - 0.5f;
	const float z = (rand() / (float) RAND_MAX) - 0.5f;
	spark.speed = (speed * 0.95f) + float3(x, y, z);
	spark.pos = pos - speed * (rand() / (float) RAND_MAX + 3) + spark.speed * 3;
	spark.size = 5.0f;
	spark.ttl = 15;

	sparks.push_front(spark);
}

void CFireBallProjectile::Collision()
{
	if (weaponDef->waterweapon && ground->GetHeightReal(pos.x, pos.z) < pos.y) {
		// make waterweapons not explode in water
		return;
	}

	CWeaponProjectile::Collision();
	deleteMe = false;
}
