/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FlareProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"

CR_BIND_DERIVED(CFlareProjectile, CProjectile, )

CR_REG_METADATA(CFlareProjectile, (
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(activateFrame),
	CR_MEMBER(deathFrame),

	CR_MEMBER(numSubProjs),
	CR_MEMBER(maxSubProjs),
	CR_MEMBER(lastSubProj),
	CR_MEMBER(subProjPos),
	CR_MEMBER(subProjVel),
	CR_MEMBER(alphaFalloff)
))

CFlareProjectile::CFlareProjectile(const float3& pos, const float3& speed, CUnit* owner, int activateFrame):
	// these are synced, but neither weapon nor piece
	// (only created by units that can drop flares)
	CProjectile(pos, speed, owner, true, false, false),

	activateFrame(activateFrame),
	deathFrame(activateFrame + ((owner != nullptr)? owner->unitDef->flareTime: 1)),

	numSubProjs(0),
	maxSubProjs(std::min((owner != nullptr)? owner->unitDef->flareSalvoSize: 0, int(subProjPos.size()))),
	lastSubProj(0)
{
	alphaFalloff = (owner != nullptr)? 1.0f / owner->unitDef->flareTime: 1.0f;
	checkCol = false;
	useAirLos = true;

	SetRadiusAndHeight(45.0f, 0.0f);

	// flares fall slower
	mygravity *= 0.3f;
}


void CFlareProjectile::Update()
{
	const CUnit* owner = CProjectile::owner();
	const UnitDef* ownerDef = (owner != nullptr)? owner->unitDef: nullptr;

	if (gs->frameNum == activateFrame) {
		if (owner != nullptr) {
			SetPosition(owner->pos);
			CWorldObject::SetVelocity(owner->speed);
			CWorldObject::SetVelocity(speed + (owner->rightdir * ownerDef->flareDropVector.x));
			CWorldObject::SetVelocity(speed + (owner->updir    * ownerDef->flareDropVector.y));
			CWorldObject::SetVelocity(speed + (owner->frontdir * ownerDef->flareDropVector.z));
			SetVelocityAndSpeed(speed);
		} else {
			deleteMe = true;
		}
	}
	if (gs->frameNum >= activateFrame) {
		SetPosition(pos + speed);
		SetVelocityAndSpeed((speed * 0.95f) + (UpVector * mygravity));

		//FIXME: just spawn new flares, if new missiles incoming?
		if (owner != nullptr && lastSubProj < (gs->frameNum - ownerDef->flareSalvoDelay) && numSubProjs < maxSubProjs) {
			subProjPos[numSubProjs] = owner->pos;
			subProjVel[numSubProjs] = owner->speed;

			subProjVel[numSubProjs] += (owner->rightdir * ownerDef->flareDropVector.x);
			subProjVel[numSubProjs] += (owner->updir    * ownerDef->flareDropVector.y);
			subProjVel[numSubProjs] += (owner->frontdir * ownerDef->flareDropVector.z);

			numSubProjs += 1;
			lastSubProj = gs->frameNum;

			// try to trick at least one missile into retargeting us
			// (this means flares can steal missiles from each other)
			for (CMissileProjectile* missile: owner->incomingMissiles) {
				if (missile == nullptr)
					continue;

				if (gsRNG.NextFloat() >= ownerDef->flareEfficiency)
					continue;

				missile->SetTargetObject(this);
				missile->AddDeathDependence(this, DEPENDENCE_DECOYTARGET);
			}
		}

		for (int a = 0; a < numSubProjs; ++a) {
			subProjPos[a] += subProjVel[a];
			subProjVel[a] *= 0.95f;
			subProjVel[a].y += mygravity;
		}
	}

	deleteMe |= (gs->frameNum >= deathFrame);
}

void CFlareProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	if (gs->frameNum <= activateFrame)
		return;

	constexpr float radius = 6.0f;
	const     float alpha = std::max(0.0f, 1.0f - (gs->frameNum - activateFrame) * alphaFalloff);

	unsigned char col[4];
	col[0] = (unsigned char) (alpha * 1.0f) * 255;
	col[1] = (unsigned char) (alpha * 0.5f) * 255;
	col[2] = (unsigned char) (alpha * 0.2f) * 255;
	col[3] = 1;

	for (int a = 0; a < numSubProjs; ++a) {
		const float3 interPos = subProjPos[a] + subProjVel[a] * globalRendering->timeOffset;

		#define fpt projectileDrawer->flareprojectiletex
		va->SafeAppend({interPos - camera->GetRight() * radius - camera->GetUp() * radius, fpt->xstart, fpt->ystart, col});
		va->SafeAppend({interPos + camera->GetRight() * radius - camera->GetUp() * radius, fpt->xend,   fpt->ystart, col});
		va->SafeAppend({interPos + camera->GetRight() * radius + camera->GetUp() * radius, fpt->xend,   fpt->yend,   col});

		va->SafeAppend({interPos + camera->GetRight() * radius + camera->GetUp() * radius, fpt->xend,   fpt->yend,   col});
		va->SafeAppend({interPos - camera->GetRight() * radius + camera->GetUp() * radius, fpt->xstart, fpt->yend,   col});
		va->SafeAppend({interPos - camera->GetRight() * radius - camera->GetUp() * radius, fpt->xstart, fpt->ystart, col});
		#undef fpt
	}
}

