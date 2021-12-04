/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimpleParticleSystem.h"

#include "GenericParticleProjectile.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/ColorMap.h"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "System/creg/DefTypes.h"
#include "System/float3.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"

CR_BIND_DERIVED(CSimpleParticleSystem, CProjectile, )

CR_REG_METADATA(CSimpleParticleSystem,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(emitVector),
		CR_MEMBER(emitMul),
		CR_MEMBER(gravity),
		CR_MEMBER(colorMap),
		CR_MEMBER(texture),
		CR_MEMBER(airdrag),
		CR_MEMBER(particleLife),
		CR_MEMBER(particleLifeSpread),
		CR_MEMBER(numParticles),
		CR_MEMBER(particleSpeed),
		CR_MEMBER(particleSpeedSpread),
		CR_MEMBER(particleSize),
		CR_MEMBER(particleSizeSpread),
		CR_MEMBER(emitRot),
		CR_MEMBER(emitRotSpread),
		CR_MEMBER(directional),
		CR_MEMBER(sizeGrowth),
		CR_MEMBER(sizeMod),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_MEMBER(particles)
))

CR_BIND(CSimpleParticleSystem::Particle, )

CR_REG_METADATA_SUB(CSimpleParticleSystem, Particle,
(
	CR_MEMBER(pos),
	CR_MEMBER(life),
	CR_MEMBER(speed),
	CR_MEMBER(decayrate),
	CR_MEMBER(size),
	CR_MEMBER(sizeGrowth),
	CR_MEMBER(sizeMod)
))

CSimpleParticleSystem::CSimpleParticleSystem()
	: emitVector(ZeroVector)
	, emitMul(1.0f, 1.0f, 1.0f)
	, gravity(ZeroVector)
	, particleSpeed(0.0f)
	, particleSpeedSpread(0.0f)
	, emitRot(0.0f)
	, emitRotSpread(0.0f)
	, texture(nullptr)
	, colorMap(nullptr)
	, directional(false)
	, particleLife(0.0f)
	, particleLifeSpread(0.0f)
	, particleSize(0.0f)
	, particleSizeSpread(0.0f)
	, airdrag(0.0f)
	, sizeGrowth(0.0f)
	, sizeMod(0.0f)
	, numParticles(0)
{
	checkCol = false;
	useAirLos = true;
}

void CSimpleParticleSystem::Draw(GL::RenderDataBufferTC* va) const
{
	if (directional) {
		for (int i = 0; i < numParticles; i++) {
			const Particle* p = &particles[i];

			if (p->life >= 1.0f)
				continue;

			const float3 zdir = (p->pos - camera->GetPos()).SafeANormalize();
			const float3 ydir = (zdir.cross(p->speed)).SafeANormalize();
			const float3 xdir = (zdir.cross(ydir));

			const float3 interPos = p->pos + p->speed * globalRendering->timeOffset;
			const float size = p->size;

			unsigned char color[4];
			colorMap->GetColor(color, p->life);

			if (p->speed.SqLength() > 0.001f) {
				va->SafeAppend({interPos - ydir * size - xdir * size, texture->xstart, texture->ystart, color});
				va->SafeAppend({interPos - ydir * size + xdir * size, texture->xend,   texture->ystart, color});
				va->SafeAppend({interPos + ydir * size + xdir * size, texture->xend,   texture->yend,   color});

				va->SafeAppend({interPos + ydir * size + xdir * size, texture->xend,   texture->yend,   color});
				va->SafeAppend({interPos + ydir * size - xdir * size, texture->xstart, texture->yend,   color});
				va->SafeAppend({interPos - ydir * size - xdir * size, texture->xstart, texture->ystart, color});
			} else {
				// in this case the particle's coor-system is degenerate
				va->SafeAppend({interPos - camera->GetUp() * size - camera->GetRight() * size, texture->xstart, texture->ystart, color});
				va->SafeAppend({interPos - camera->GetUp() * size + camera->GetRight() * size, texture->xend,   texture->ystart, color});
				va->SafeAppend({interPos + camera->GetUp() * size + camera->GetRight() * size, texture->xend,   texture->yend,   color});

				va->SafeAppend({interPos + camera->GetUp() * size + camera->GetRight() * size, texture->xend,   texture->yend,   color});
				va->SafeAppend({interPos + camera->GetUp() * size - camera->GetRight() * size, texture->xstart, texture->yend,   color});
				va->SafeAppend({interPos - camera->GetUp() * size - camera->GetRight() * size, texture->xstart, texture->ystart, color});
			}
		}
	} else {
		for (int i = 0; i < numParticles; i++) {
			const Particle* p = &particles[i];

			if (p->life >= 1.0f)
				continue;

			unsigned char color[4];
			colorMap->GetColor(color, p->life);

			const float3 interPos = p->pos + p->speed * globalRendering->timeOffset;
			const float3 cameraRight = camera->GetRight() * p->size;
			const float3 cameraUp    = camera->GetUp() * p->size;

			va->SafeAppend({interPos - cameraRight - cameraUp, texture->xstart, texture->ystart, color});
			va->SafeAppend({interPos + cameraRight - cameraUp, texture->xend,   texture->ystart, color});
			va->SafeAppend({interPos + cameraRight + cameraUp, texture->xend,   texture->yend,   color});

			va->SafeAppend({interPos + cameraRight + cameraUp, texture->xend,   texture->yend,   color});
			va->SafeAppend({interPos - cameraRight + cameraUp, texture->xstart, texture->yend,   color});
			va->SafeAppend({interPos - cameraRight - cameraUp, texture->xstart, texture->ystart, color});
		}
	}
}

void CSimpleParticleSystem::Update()
{
	deleteMe = true;

	for (auto& p: particles) {
		if (p.life < 1.0f) {
			p.pos   += p.speed;
			p.speed += gravity;
			p.speed *= airdrag;
			p.life  += p.decayrate;
			p.size   = p.size * sizeMod + sizeGrowth;

			deleteMe = false;
		}
	}
}

void CSimpleParticleSystem::Init(const CUnit* owner, const float3& offset)
{
	CProjectile::Init(owner, offset);

	const float3 up = emitVector;
	const float3 right = up.cross(float3(up.y, up.z, -up.x));
	const float3 forward = up.cross(right);

	// FIXME: should catch these earlier and for more projectile-types
	if (colorMap == nullptr) {
		colorMap = CColorMap::LoadFromFloatVector(std::vector<float>(8, 1.0f));
		LOG_L(L_WARNING, "[CSimpleParticleSystem::%s] no color-map specified", __FUNCTION__);
	}
	if (texture == nullptr) {
		texture = &projectileDrawer->textureAtlas->GetTexture("simpleparticle");
		LOG_L(L_WARNING, "[CSimpleParticleSystem::%s] no texture specified", __FUNCTION__);
	}

	particles.resize(numParticles);
	for (auto& p: particles) {
		float az = guRNG.NextFloat() * math::TWOPI;
		float ay = (emitRot + (emitRotSpread * guRNG.NextFloat())) * math::DEG_TO_RAD;

		p.pos = offset;
		p.speed = ((up * emitMul.y) * fastmath::cos(ay) - ((right * emitMul.x) * fastmath::cos(az) - (forward * emitMul.z) * fastmath::sin(az)) * fastmath::sin(ay)) * (particleSpeed + (guRNG.NextFloat() * particleSpeedSpread));
		p.life = 0;
		p.decayrate = 1.0f / (particleLife + (guRNG.NextFloat() * particleLifeSpread));
		p.size = particleSize + guRNG.NextFloat()*particleSizeSpread;
	}

	drawRadius = (particleSpeed + particleSpeedSpread) * (particleLife * particleLifeSpread);
}



bool CSimpleParticleSystem::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_FLOAT3(CSimpleParticleSystem, emitVector         )
	CHECK_MEMBER_INFO_FLOAT3(CSimpleParticleSystem, emitMul            )
	CHECK_MEMBER_INFO_FLOAT3(CSimpleParticleSystem, gravity            )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, particleSpeed      )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, particleSpeedSpread)
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, emitRot            )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, emitRotSpread      )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, particleLife       )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, particleLifeSpread )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, particleSize       )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, particleSizeSpread )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, airdrag            )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, sizeGrowth         )
	CHECK_MEMBER_INFO_FLOAT (CSimpleParticleSystem, sizeMod            )
	CHECK_MEMBER_INFO_INT   (CSimpleParticleSystem, numParticles       )
	CHECK_MEMBER_INFO_BOOL  (CSimpleParticleSystem, directional        )
	CHECK_MEMBER_INFO_PTR   (CSimpleParticleSystem, texture , projectileDrawer->textureAtlas->GetTexturePtr)
	CHECK_MEMBER_INFO_PTR   (CSimpleParticleSystem, colorMap, CColorMap::LoadFromDefString                 )

	return false;
}


CR_BIND_DERIVED(CSphereParticleSpawner, CSimpleParticleSystem, )

CR_REG_METADATA(CSphereParticleSpawner, )

void CSphereParticleSpawner::Init(const CUnit* owner, const float3& offset)
{
	const float3 up = emitVector;
	const float3 right = up.cross(float3(up.y, up.z, -up.x));
	const float3 forward = up.cross(right);

	// FIXME: should catch these earlier and for more projectile-types
	if (colorMap == nullptr) {
		colorMap = CColorMap::LoadFromFloatVector(std::vector<float>(8, 1.0f));
		LOG_L(L_WARNING, "[CSphereParticleSpawner::%s] no color-map specified", __FUNCTION__);
	}
	if (texture == nullptr) {
		texture = &projectileDrawer->textureAtlas->GetTexture("sphereparticle");
		LOG_L(L_WARNING, "[CSphereParticleSpawner::%s] no texture specified", __FUNCTION__);
	}

	for (int i = 0; i < numParticles; i++) {
		const float az = guRNG.NextFloat() * math::TWOPI;
		const float ay = (emitRot + emitRotSpread*guRNG.NextFloat()) * math::DEG_TO_RAD;

		const float3 pspeed = ((up * emitMul.y) * std::cos(ay) - ((right * emitMul.x) * std::cos(az) - (forward * emitMul.z) * std::sin(az)) * std::sin(ay)) * (particleSpeed + (guRNG.NextFloat() * particleSpeedSpread));

		CGenericParticleProjectile* particle = projMemPool.alloc<CGenericParticleProjectile>(owner, pos + offset, pspeed);

		particle->decayrate = 1.0f / (particleLife + guRNG.NextFloat() * particleLifeSpread);
		particle->life = 0;
		particle->size = particleSize + guRNG.NextFloat() * particleSizeSpread;

		particle->texture = texture;
		particle->colorMap = colorMap;

		particle->airdrag = airdrag;
		particle->sizeGrowth = sizeGrowth;
		particle->sizeMod = sizeMod;

		particle->gravity = gravity;
		particle->directional = directional;

		particle->SetRadiusAndHeight(particle->size + sizeGrowth * particleLife, 0.0f);
	}

	deleteMe = true;
}

bool CSphereParticleSpawner::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	return CSimpleParticleSystem::GetMemberInfo(memberInfo);
}
