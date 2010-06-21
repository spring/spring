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

CR_BIND_DERIVED(CExplosiveProjectile, CWeaponProjectile, (float3(0, 0, 0), float3(0, 0, 0), NULL, NULL, 1, 0));

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
	CUnit* owner, const WeaponDef *weaponDef,
	int ttl, float areaOfEffect, float g):

	CWeaponProjectile(pos, speed, owner, 0, ZeroVector, weaponDef, 0, ttl),
	areaOfEffect(areaOfEffect),
	curTime(0)
{
	projectileType = WEAPON_EXPLOSIVE_PROJECTILE;

	//! either map or weaponDef gravity
	mygravity = g;
	useAirLos = true;

	if (weaponDef) {
		SetRadius(weaponDef->collisionSize);
		drawRadius = weaponDef->size;
	}

	if (ttl <= 0)
		invttl = 1;
	else
		invttl = 1.0f / ttl;

#ifdef TRACE_SYNC
	tracefile << "New explosive: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif


	if (!cegTag.empty()) {
		ceg.Load(explGenHandler, cegTag);
	}
}

CExplosiveProjectile::~CExplosiveProjectile()
{

}

void CExplosiveProjectile::Update()
{
	if (!luaMoveCtrl) {
		pos += speed;
		speed.y += mygravity;
	}

	if (--ttl == 0) {
		Collision();
	} else {
		if (!cegTag.empty() && ttl > 0) {
			ceg.Explosion(pos, ttl, areaOfEffect, 0x0, 0.0f, 0x0, speed);
		}
	}

	if (weaponDef->noExplode) {
		if (TraveledRange())
			CProjectile::Collision();
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
		float h=ground->GetHeight2(pos.x,pos.z);
		if(h>pos.y){
			float3 n=ground->GetNormal(pos.x,pos.z);
			pos-=speed*std::max(0.0f,std::min(1.0f,float((h-pos.y)*n.y/n.dot(speed)+0.1f)));
		}
		else if (weaponDef->waterweapon) {
			return; //let waterweapons go underwater
		}
	}

	CWeaponProjectile::Collision();
}

void CExplosiveProjectile::Collision(CUnit *unit)
{
	CWeaponProjectile::Collision(unit);
}

void CExplosiveProjectile::Draw(void)
{
	if (model) { //dont draw if a 3d model has been defined for us
		return;
	}

	inArray = true;
	unsigned char col[4];
	if (weaponDef->visuals.colorMap) {
		weaponDef->visuals.colorMap->GetColor(col, curTime);
	} else {
		col[0] = int(weaponDef->visuals.color.x * 255);
		col[1] = int(weaponDef->visuals.color.y * 255);
		col[2] = int(weaponDef->visuals.color.z * 255);
		col[3] = int(weaponDef->intensity * 255);
	}

	const AtlasedTexture* tex = weaponDef->visuals.texture1;
	const float  alphaDecay = weaponDef->visuals.alphaDecay;
	const float  sizeDecay  = weaponDef->visuals.sizeDecay;
	const float  separation = weaponDef->visuals.separation;
	const bool   noGap      = weaponDef->visuals.noGap;
	const int    stages     = weaponDef->visuals.stages;
	const float  invStages  = 1.0f / (float)stages;

	const float3 ndir = dir * separation * 0.6f;

	va->EnlargeArrays(stages * 4,0, VA_SIZE_TC);

	for (int a = 0; a < stages; ++a) { //! CAUTION: loop count must match EnlargeArrays above
		const float aDecay = (stages - (a * alphaDecay)) * invStages;
		col[0] = int(aDecay * col[0]);
		col[1] = int(aDecay * col[1]);
		col[2] = int(aDecay * col[2]);
		col[3] = int(aDecay * col[3]);

		const float  size  = drawRadius * (1.0f - (a * sizeDecay));
		const float3 up    = camera->up    * size;
		const float3 right = camera->right * size;
		const float3 interPos2 = drawPos - ((noGap)? (ndir * size * a): (ndir * drawRadius * a));

		va->AddVertexQTC(interPos2 - right - up, tex->xstart, tex->ystart, col);
		va->AddVertexQTC(interPos2 + right - up, tex->xend,   tex->ystart, col);
		va->AddVertexQTC(interPos2 + right + up, tex->xend,   tex->yend,   col);
		va->AddVertexQTC(interPos2 - right + up, tex->xstart, tex->yend,   col);
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
