/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FireProjectile.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "System/creg/STL_Deque.h"

CR_BIND_DERIVED(CFireProjectile, CProjectile, )
CR_BIND(CFireProjectile::SubParticle, )

CR_REG_METADATA(CFireProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(ttl),
	CR_MEMBER(emitPos),
	CR_MEMBER(emitRadius),
	CR_MEMBER(particleTime),
	CR_MEMBER(particleSize),
	CR_MEMBER(ageSpeed),
	CR_MEMBER(subParticles2),
	CR_MEMBER(subParticles)
))

CR_REG_METADATA_SUB(CFireProjectile, SubParticle, (
	CR_MEMBER(pos),
	CR_MEMBER(posDif),
	CR_MEMBER(age),
	CR_MEMBER(maxSize),
	CR_MEMBER(rotSpeed),
	CR_MEMBER(smokeType)
))

CFireProjectile::CFireProjectile(
	const float3& pos,
	const float3& spd,
	CUnit* owner,
	int emitTtl,
	int particleTtl,
	float emitRadius,
	float particleSize
):
	// these are synced, but neither weapon nor piece
	// (only burning features create instances of them)
	CProjectile(pos, speed, owner, true, false, false),
	ttl(emitTtl),
	emitPos(pos),
	emitRadius(emitRadius),
	particleTime(particleTtl),
	particleSize(particleSize)
{
	drawRadius = emitRadius + particleTime * speed.w;
	checkCol = false;
	ageSpeed = 1.0f / particleTime;

	SetPosition(pos + (UpVector * particleTime * speed.w * 0.5f));

	alwaysVisible = true;
	castShadow = true;
}

void CFireProjectile::Update()
{
	if ((--ttl) > 0) {
		const float partSat = (gs->frameNum & 1) ? 1.0f : 0.8f;
		if (projectileHandler.GetParticleSaturation() < partSat) {
			// unsynced code
			SubParticle sub;
			sub.age = 0;
			sub.maxSize = (0.7f + guRNG.NextFloat()*0.3f) * particleSize;
			sub.posDif = guRNG.NextVector() * emitRadius;
			sub.pos = emitPos;
			sub.pos.y += sub.posDif.y;
			sub.posDif.y = 0;
			sub.rotSpeed = (guRNG.NextFloat() - 0.5f) * 4;
			sub.smokeType = guRNG.NextInt(projectileDrawer->NumSmokeTextures());
			subParticles.push_front(sub);

			sub.maxSize = (0.7f + guRNG.NextFloat()*0.3f) * particleSize;
			sub.posDif = guRNG.NextVector() * emitRadius;
			sub.pos = emitPos;
			sub.pos.y += sub.posDif.y - radius*0.3f;
			sub.posDif.y = 0;
			sub.rotSpeed=(guRNG.NextFloat() - 0.5f) * 4;
			subParticles2.push_front(sub);
		}
		if (!(ttl & 31)) {
			// copy on purpose, since the below can call Lua
			QuadFieldQuery qfQuery;
			quadField.GetFeaturesExact(qfQuery, emitPos + envResHandler.GetCurrentWindVec() * 0.7f, emitRadius * 2);
			quadField.GetUnitsExact(qfQuery, emitPos + envResHandler.GetCurrentWindVec() * 0.7f, emitRadius * 2);

			for (CFeature* f: *qfQuery.features) {
				if (gsRNG.NextFloat() > 0.8f) {
					f->StartFire();
				}
			}

			const DamageArray fireDmg(30);
			for (CUnit* u: *qfQuery.units) {
				u->DoDamage(fireDmg, ZeroVector, NULL, -CSolidObject::DAMAGE_EXTSOURCE_FIRE, -1);
			}
		}
	}

	for (SubParticle& pi: subParticles) {
		if ((pi.age += ageSpeed) > 1.0f) {
			subParticles.pop_back();
			break;
		}

		pi.pos += (speed + envResHandler.GetCurrentWindVec() * pi.age * 0.05f + pi.posDif * 0.1f);
		pi.posDif *= 0.9f;
	}

	for (SubParticle& pi: subParticles2) {
		if ((pi.age += (ageSpeed * 1.5f)) > 1.0f) {
			subParticles2.pop_back();
			break;
		}

		pi.pos += (speed * 0.7f + pi.posDif * 0.1f);
		pi.posDif *= 0.9f;
	}

	deleteMe |= (ttl <= -particleTime);
}

void CFireProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	unsigned char col [4]; col[3] = 1;
	unsigned char col2[4];

	for (const SubParticle& pi: subParticles2) {
		const float  age = pi.age + ageSpeed * globalRendering->timeOffset;
		const float size = pi.maxSize * age;
		const float  rot = pi.rotSpeed * age;

		const float sinRot = fastmath::sin(rot);
		const float cosRot = fastmath::cos(rot);

		float3 dir1 = (camera->GetRight()*cosRot + camera->GetUp()*sinRot) * size;
		float3 dir2 = (camera->GetRight()*sinRot - camera->GetUp()*cosRot) * size;

		float3 interPos = pi.pos;

		col[0] = (unsigned char) ((1 - age) * 255);
		col[1] = (unsigned char) ((1 - age) * 255);
		col[2] = (unsigned char) ((1 - age) * 255);

		va->SafeAppend({interPos - dir1 - dir2, projectileDrawer->explotex->xstart, projectileDrawer->explotex->ystart, col});
		va->SafeAppend({interPos + dir1 - dir2, projectileDrawer->explotex->xend,   projectileDrawer->explotex->ystart, col});
		va->SafeAppend({interPos + dir1 + dir2, projectileDrawer->explotex->xend,   projectileDrawer->explotex->yend,   col});

		va->SafeAppend({interPos + dir1 + dir2, projectileDrawer->explotex->xend,   projectileDrawer->explotex->yend,   col});
		va->SafeAppend({interPos - dir1 + dir2, projectileDrawer->explotex->xstart, projectileDrawer->explotex->yend,   col});
		va->SafeAppend({interPos - dir1 - dir2, projectileDrawer->explotex->xstart, projectileDrawer->explotex->ystart, col});
	}

	for (const SubParticle& pi: subParticles) {
		const AtlasedTexture* at = projectileDrawer->GetSmokeTexture(pi.smokeType);

		const float  age = pi.age + ageSpeed * globalRendering->timeOffset;
		const float size = pi.maxSize * fastmath::apxsqrt(age);
		const float  rot = pi.rotSpeed * age;

		const float sinRot = fastmath::sin(rot);
		const float cosRot = fastmath::cos(rot);

		float3 dir1 = (camera->GetRight()*cosRot + camera->GetUp()*sinRot) * size;
		float3 dir2 = (camera->GetRight()*sinRot - camera->GetUp()*cosRot) * size;

		float3 interPos = pi.pos;

		if (age < (1.0f / 1.31f)) {
			col[0] = (unsigned char) ((1 - age * 1.3f) * 255);
			col[1] = (unsigned char) ((1 - age * 1.3f) * 255);
			col[2] = (unsigned char) ((1 - age * 1.3f) * 255);
			col[3] = 1;

			va->SafeAppend({interPos - dir1 - dir2, projectileDrawer->explotex->xstart, projectileDrawer->explotex->ystart, col});
			va->SafeAppend({interPos + dir1 - dir2, projectileDrawer->explotex->xend,   projectileDrawer->explotex->ystart, col});
			va->SafeAppend({interPos + dir1 + dir2, projectileDrawer->explotex->xend,   projectileDrawer->explotex->yend,   col});

			va->SafeAppend({interPos + dir1 + dir2, projectileDrawer->explotex->xend,   projectileDrawer->explotex->yend,   col});
			va->SafeAppend({interPos - dir1 + dir2, projectileDrawer->explotex->xstart, projectileDrawer->explotex->yend,   col});
			va->SafeAppend({interPos - dir1 - dir2, projectileDrawer->explotex->xstart, projectileDrawer->explotex->ystart, col});
		}

		unsigned char c;
		if (age < 0.5f) {
			c = (unsigned char)        (age * 510);
		} else {
			c = (unsigned char) (510 - (age * 510));
		}
		col2[0] = (unsigned char) (c * 0.6f);
		col2[1] = (unsigned char) (c * 0.6f);
		col2[2] = (unsigned char) (c * 0.6f);
		col2[3] = c;

		va->SafeAppend({interPos - dir1 - dir2, at->xstart, at->ystart, col2});
		va->SafeAppend({interPos + dir1 - dir2, at->xend,   at->ystart, col2});
		va->SafeAppend({interPos + dir1 + dir2, at->xend,   at->yend,   col2});

		va->SafeAppend({interPos + dir1 + dir2, at->xend,   at->yend,   col2});
		va->SafeAppend({interPos - dir1 + dir2, at->xstart, at->yend,   col2});
		va->SafeAppend({interPos - dir1 - dir2, at->xstart, at->ystart, col2});
	}
}

