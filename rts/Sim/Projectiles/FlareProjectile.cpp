/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FlareProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"

CR_BIND_DERIVED(CFlareProjectile, CProjectile, )

CR_REG_METADATA(CFlareProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(activateFrame),
	CR_MEMBER(deathFrame),

	CR_MEMBER(numSub),
	CR_MEMBER(lastSub),
	CR_MEMBER(subPos),
	CR_MEMBER(subSpeed),
	CR_MEMBER(alphaFalloff)
))

CFlareProjectile::CFlareProjectile(const float3& pos, const float3& speed, CUnit* owner, int activateFrame):
	//! these are synced, but neither weapon nor piece
	//! (only created by units that can drop flares)
	CProjectile(pos, speed, owner, true, false, false),
	activateFrame(activateFrame),
	deathFrame(activateFrame + (owner? owner->unitDef->flareTime: 1)),
	numSub(0),
	lastSub(0)
{
	alphaFalloff = owner? 1.0f / owner->unitDef->flareTime: 1.0f;
	checkCol = false;
	useAirLos = true;

	SetRadiusAndHeight(45.0f, 0.0f);

	subPos.resize(owner->unitDef->flareSalvoSize);
	subSpeed.resize(owner->unitDef->flareSalvoSize);
	mygravity *= 0.3f; //! flares fall slower
}

CFlareProjectile::~CFlareProjectile()
{
}

void CFlareProjectile::Update()
{
	CUnit* owner = CProjectile::owner();

	if (gs->frameNum == activateFrame) {
		if (owner != NULL) {
			SetPosition(owner->pos);
			CWorldObject::SetVelocity(owner->speed);
			CWorldObject::SetVelocity(speed + (owner->rightdir * owner->unitDef->flareDropVector.x));
			CWorldObject::SetVelocity(speed + (owner->updir    * owner->unitDef->flareDropVector.y));
			CWorldObject::SetVelocity(speed + (owner->frontdir * owner->unitDef->flareDropVector.z));
			SetVelocityAndSpeed(speed);
		} else {
			deleteMe = true;
		}
	}
	if (gs->frameNum >= activateFrame) {
		SetPosition(pos + speed);
		SetVelocityAndSpeed((speed * 0.95f) + (UpVector * mygravity));

		//FIXME: just spawn new flares, if new missiles incoming?
		if(owner && lastSub < (gs->frameNum - owner->unitDef->flareSalvoDelay) && numSub<owner->unitDef->flareSalvoSize) {
			subPos[numSub] = owner->pos;
			float3 s = owner->speed;
			s += owner->rightdir * owner->unitDef->flareDropVector.x;
			s += owner->updir * owner->unitDef->flareDropVector.y;
			s += owner->frontdir * owner->unitDef->flareDropVector.z;
			subSpeed[numSub] = s;
			++numSub;
			lastSub = gs->frameNum;

			for (CMissileProjectile* missile: owner->incomingMissiles) {
				if (gs->randFloat() < owner->unitDef->flareEfficiency) {
					missile->SetTargetObject(this);
					missile->AddDeathDependence(this, DEPENDENCE_DECOYTARGET);
				}
			}
		}
		for (int a = 0; a < numSub; ++a) {
			subPos[a] += subSpeed[a];
			subSpeed[a] *= 0.95f;
			subSpeed[a].y += mygravity;
		}
	}

	if (gs->frameNum >= deathFrame) {
		deleteMe = true;
	}
}

void CFlareProjectile::Draw()
{
	if (gs->frameNum <= activateFrame) {
		return;
	}

	inArray = true;
	unsigned char col[4];
	float alpha = std::max(0.0f,1-(gs->frameNum-activateFrame)*alphaFalloff);
	col[0] = (unsigned char) (alpha * 255);
	col[1] = (unsigned char) (alpha * 0.5f) * 255;
	col[2] = (unsigned char) (alpha * 0.2f) * 255;
	col[3] = 1;

	float rad = 6.0;
	va->EnlargeArrays(numSub * 4, 0, VA_SIZE_TC);
	for (int a = 0; a < numSub; ++a) {
		//! CAUTION: loop count must match EnlargeArrays above
		const float3 interPos = subPos[a] + subSpeed[a] * globalRendering->timeOffset;

		#define fpt projectileDrawer->flareprojectiletex
		va->AddVertexQTC(interPos - camera->GetRight() * rad - camera->GetUp() * rad, fpt->xstart, fpt->ystart, col);
		va->AddVertexQTC(interPos + camera->GetRight() * rad - camera->GetUp() * rad, fpt->xend,   fpt->ystart, col);
		va->AddVertexQTC(interPos + camera->GetRight() * rad + camera->GetUp() * rad, fpt->xend,   fpt->yend,   col);
		va->AddVertexQTC(interPos - camera->GetRight() * rad + camera->GetUp() * rad, fpt->xstart, fpt->yend,   col);
		#undef fpt
	}
}


int CFlareProjectile::GetProjectilesCount() const
{
	return subPos.size();
}
