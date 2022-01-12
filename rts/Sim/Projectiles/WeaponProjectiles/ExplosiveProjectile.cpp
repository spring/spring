/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ExplosiveProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

CR_BIND_DERIVED(CExplosiveProjectile, CWeaponProjectile, )

CR_REG_METADATA(CExplosiveProjectile, (
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(invttl),
	CR_MEMBER(curTime)
))


CExplosiveProjectile::CExplosiveProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	, invttl(0.0f)
	, curTime(0.0f)
{
	projectileType = WEAPON_EXPLOSIVE_PROJECTILE;

	mygravity = params.gravity;
	useAirLos = true;

	if (weaponDef != nullptr) {
		SetRadiusAndHeight(weaponDef->collisionSize, 0.0f);
		drawRadius = weaponDef->size;
	}

	if (ttl <= 0) {
		invttl = 1.0f;
	} else {
		invttl = 1.0f / ttl;
	}
}

void CExplosiveProjectile::Update()
{
	CProjectile::Update();

	if (--ttl == 0) {
		Collision();
	} else {
		if (ttl > 0)
			explGenHandler.GenExplosion(cegID, pos, speed, ttl, damages->damageAreaOfEffect, 0.0f, nullptr, nullptr);
	}

	curTime += invttl;
	curTime = std::min(curTime, 1.0f);

	if (weaponDef->noExplode && TraveledRange()) {
		CProjectile::Collision();
		return;
	}

	UpdateGroundBounce();
	UpdateInterception();
}

void CExplosiveProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	// do not draw if a 3D model has been defined for us
	if (model != nullptr)
		return;

	unsigned char col[4] = {0};

	const WeaponDef::Visuals& wdVisuals = weaponDef->visuals;
	const AtlasedTexture* tex = wdVisuals.texture1;

	if (wdVisuals.colorMap != nullptr) {
		wdVisuals.colorMap->GetColor(col, curTime);
	} else {
		col[0] = wdVisuals.color.x    * 255;
		col[1] = wdVisuals.color.y    * 255;
		col[2] = wdVisuals.color.z    * 255;
		col[3] = weaponDef->intensity * 255;
	}

	const float  alphaDecay = wdVisuals.alphaDecay;
	const float  sizeDecay  = wdVisuals.sizeDecay;
	const float  separation = wdVisuals.separation;
	const bool   noGap      = wdVisuals.noGap;
	const int    stages     = wdVisuals.stages;
	const float  invStages  = 1.0f / std::max(1, stages);

	const float3 ndir = dir * separation * 0.6f;

	for (int stage = 0; stage < stages; ++stage) {
		const float stageDecay = (stages - (stage * alphaDecay)) * invStages;
		const float stageSize  = drawRadius * (1.0f - (stage * sizeDecay));

		const float3 ydirCam  = camera->GetUp()    * stageSize;
		const float3 xdirCam  = camera->GetRight() * stageSize;
		const float3 stageGap = (noGap)? (ndir * stageSize * stage): (ndir * drawRadius * stage);
		const float3 stagePos = drawPos - stageGap;

		col[0] = stageDecay * col[0];
		col[1] = stageDecay * col[1];
		col[2] = stageDecay * col[2];
		col[3] = stageDecay * col[3];

		va->SafeAppend({stagePos - xdirCam - ydirCam, tex->xstart, tex->ystart, col});
		va->SafeAppend({stagePos + xdirCam - ydirCam, tex->xend,   tex->ystart, col});
		va->SafeAppend({stagePos + xdirCam + ydirCam, tex->xend,   tex->yend,   col});

		va->SafeAppend({stagePos + xdirCam + ydirCam, tex->xend,   tex->yend,   col});
		va->SafeAppend({stagePos - xdirCam + ydirCam, tex->xstart, tex->yend,   col});
		va->SafeAppend({stagePos - xdirCam - ydirCam, tex->xstart, tex->ystart, col});
	}
}

int CExplosiveProjectile::ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed)
{
	if (luaMoveCtrl)
		return 0;

	const float3 rdir = (pos - shieldPos).Normalize();

	if (rdir.dot(speed) < shieldMaxSpeed) {
		SetVelocityAndSpeed(speed + (rdir * shieldForce));
		return 2;
	}

	return 0;
}

int CExplosiveProjectile::GetProjectilesCount() const
{
	return weaponDef->visuals.stages;
}
