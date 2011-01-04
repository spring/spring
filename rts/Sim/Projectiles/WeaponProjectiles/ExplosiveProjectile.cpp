/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "ExplosiveProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

#ifdef TRACE_SYNC
	#include "System/Sync/SyncTracer.h"
#endif

CR_BIND_DERIVED(CExplosiveProjectile, CWeaponProjectile, (ZeroVector, ZeroVector, NULL, NULL, 1, 0));

CR_REG_METADATA(CExplosiveProjectile, (
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(areaOfEffect),
	CR_RESERVED(16)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CExplosiveProjectile::CExplosiveProjectile(
		const float3& pos, const float3& speed,
		CUnit* owner, const WeaponDef* weaponDef,
		int ttl, float areaOfEffect, float g)
	: CWeaponProjectile(pos, speed, owner, NULL, ZeroVector, weaponDef, NULL, ttl)
	, areaOfEffect(areaOfEffect)
	, curTime(0)
{
	projectileType = WEAPON_EXPLOSIVE_PROJECTILE;

	//! either map or weaponDef gravity
	mygravity = g;
	useAirLos = true;

	if (weaponDef) {
		SetRadius(weaponDef->collisionSize);
		drawRadius = weaponDef->size;
	}

	if (ttl <= 0) {
		invttl = 1;
	} else {
		invttl = 1.0f / ttl;
	}

#ifdef TRACE_SYNC
	tracefile << "New explosive: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif

	cegID = gCEG->Load(explGenHandler, cegTag);
}

void CExplosiveProjectile::Update()
{
//	if (!luaMoveCtrl) {
//		pos += speed;
//		speed.y += mygravity;
//	}
	CProjectile::Update();

	if (--ttl == 0) {
		Collision();
	} else {
		if (ttl > 0) {
			gCEG->Explosion(cegID, pos, ttl, areaOfEffect, NULL, 0.0f, NULL, speed);
		}
	}

	if (weaponDef->noExplode) {
		if (TraveledRange()) {
			CProjectile::Collision();
		}
	}

	curTime += invttl;
	if (curTime > 1) {
		curTime = 1;
	}
	UpdateGroundBounce();
}

void CExplosiveProjectile::Collision()
{
	if (!weaponDef->noExplode) {
		const float h = ground->GetHeightReal(pos.x, pos.z);
		const float3& n = ground->GetNormal(pos.x, pos.z);

		if (h > pos.y) {
			pos -= speed * std::max(0.0f, std::min(1.0f, float((h - pos.y) * n.y / n.dot(speed) + 0.1f)));
		} else if (weaponDef->waterweapon) {
			return; //let waterweapons go underwater
		}
	}

	CWeaponProjectile::Collision();
}

void CExplosiveProjectile::Collision(CUnit* unit)
{
	CWeaponProjectile::Collision(unit);
}

void CExplosiveProjectile::Draw()
{
	if (model) {
		// do not draw if a 3D model has been defined for us
		return;
	}

	inArray = true;

	unsigned char col[4] = {0};

	if (weaponDef->visuals.colorMap) {
		weaponDef->visuals.colorMap->GetColor(col, curTime);
	} else {
		col[0] = weaponDef->visuals.color.x * 255;
		col[1] = weaponDef->visuals.color.y * 255;
		col[2] = weaponDef->visuals.color.z * 255;
		col[3] = weaponDef->intensity       * 255;
	}

	const AtlasedTexture* tex = weaponDef->visuals.texture1;
	const float  alphaDecay = weaponDef->visuals.alphaDecay;
	const float  sizeDecay  = weaponDef->visuals.sizeDecay;
	const float  separation = weaponDef->visuals.separation;
	const bool   noGap      = weaponDef->visuals.noGap;
	const int    stages     = weaponDef->visuals.stages;
	const float  invStages  = 1.0f / stages;

	const float3 ndir = dir * separation * 0.6f;

	va->EnlargeArrays(stages * 4,0, VA_SIZE_TC);

	for (int stage = 0; stage < stages; ++stage) { //! CAUTION: loop count must match EnlargeArrays above
		const float stageDecay = (stages - (stage * alphaDecay)) * invStages;
		const float stageSize  = drawRadius * (1.0f - (stage * sizeDecay));

		const float3 ydirCam  = camera->up    * stageSize;
		const float3 xdirCam  = camera->right * stageSize;
		const float3 stageGap = (noGap)? (ndir * stageSize * stage): (ndir * drawRadius * stage);
		const float3 stagePos = drawPos - stageGap;

		col[0] = stageDecay * col[0];
		col[1] = stageDecay * col[1];
		col[2] = stageDecay * col[2];
		col[3] = stageDecay * col[3];

		va->AddVertexQTC(stagePos - xdirCam - ydirCam, tex->xstart, tex->ystart, col);
		va->AddVertexQTC(stagePos + xdirCam - ydirCam, tex->xend,   tex->ystart, col);
		va->AddVertexQTC(stagePos + xdirCam + ydirCam, tex->xend,   tex->yend,   col);
		va->AddVertexQTC(stagePos - xdirCam + ydirCam, tex->xstart, tex->yend,   col);
	}
}

int CExplosiveProjectile::ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos, float shieldForce, float shieldMaxSpeed)
{
	if (!luaMoveCtrl) {
		const float3 rdir = (pos - shieldPos).Normalize();

		if (rdir.dot(speed) < shieldMaxSpeed) {
			speed += (rdir * shieldForce);
			return 2;
		}
	}

	return 0;
}
