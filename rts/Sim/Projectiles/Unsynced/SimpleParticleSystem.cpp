/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SimpleParticleSystem.h"
#include "GenericParticleProjectile.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/ColorMap.h"
#include "System/float3.h"
#include "System/Log/ILog.h"

CR_BIND_DERIVED(CSimpleParticleSystem, CProjectile, );

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
	CR_MEMBER(particles),
	CR_RESERVED(16)
));

CR_BIND(CSimpleParticleSystem::Particle, );

CR_REG_METADATA_SUB(CSimpleParticleSystem, Particle,
(
	CR_MEMBER(pos),
	CR_MEMBER(life),
	CR_MEMBER(speed),
	CR_MEMBER(decayrate),
	CR_MEMBER(size),
	CR_MEMBER(sizeGrowth),
	CR_MEMBER(sizeMod),
	CR_RESERVED(8)
));

CSimpleParticleSystem::CSimpleParticleSystem()
	: CProjectile()
	, emitVector(ZeroVector)
	, emitMul(1.0f, 1.0f, 1.0f)
	, gravity(ZeroVector)
	, particleSpeed(0.0f)
	, particleSpeedSpread(0.0f)
	, emitRot(0.0f)
	, emitRotSpread(0.0f)
	, texture(NULL)
	, colorMap(NULL)
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

void CSimpleParticleSystem::Draw()
{
	inArray = true;

	va->EnlargeArrays(numParticles * 4, 0, VA_SIZE_TC);

	if (directional) {
		for (int i = 0; i < numParticles; i++) {
			const Particle* p = &particles[i];

			if (p->life < 1.0f) {
				const float3 zdir = (p->pos - camera->GetPos()).SafeANormalize();
				const float3 ydir = (zdir.cross(p->speed)).SafeANormalize();
				const float3 xdir = (zdir.cross(ydir));

				const float3 interPos = p->pos + p->speed * globalRendering->timeOffset;
				const float size = p->size;

				unsigned char color[4];
				colorMap->GetColor(color, p->life);

				if (p->speed.SqLength() > 0.001f) {
					va->AddVertexQTC(interPos - ydir * size - xdir * size, texture->xstart, texture->ystart, color);
					va->AddVertexQTC(interPos - ydir * size + xdir * size, texture->xend,   texture->ystart, color);
					va->AddVertexQTC(interPos + ydir * size + xdir * size, texture->xend,   texture->yend,   color);
					va->AddVertexQTC(interPos + ydir * size - xdir * size, texture->xstart, texture->yend,   color);
				} else {
					// in this case the particle's coor-system is degenerate
					va->AddVertexQTC(interPos - camera->up * size - camera->right * size, texture->xstart, texture->ystart, color);
					va->AddVertexQTC(interPos - camera->up * size + camera->right * size, texture->xend,   texture->ystart, color);
					va->AddVertexQTC(interPos + camera->up * size + camera->right * size, texture->xend,   texture->yend,   color);
					va->AddVertexQTC(interPos + camera->up * size - camera->right * size, texture->xstart, texture->yend,   color);
				}
			}
		}
	} else {
		for (int i = 0; i < numParticles; i++) {
			const Particle* p = &particles[i];

			if (p->life < 1.0f) {
				unsigned char color[4];
				colorMap->GetColor(color, p->life);

				const float3 interPos = p->pos + p->speed * globalRendering->timeOffset;
				const float3 cameraRight = camera->right * p->size;
				const float3 cameraUp    = camera->up * p->size;

				va->AddVertexQTC(interPos - cameraRight - cameraUp, texture->xstart, texture->ystart, color);
				va->AddVertexQTC(interPos + cameraRight - cameraUp, texture->xend,   texture->ystart, color);
				va->AddVertexQTC(interPos + cameraRight + cameraUp, texture->xend,   texture->yend,   color);
				va->AddVertexQTC(interPos - cameraRight + cameraUp, texture->xstart, texture->yend,   color);
			}
		}
	}
}

void CSimpleParticleSystem::Update()
{
	deleteMe = true;

	for (int i = 0; i < numParticles; i++) {
		if (particles[i].life < 1.0f) {
			particles[i].pos += particles[i].speed;
			particles[i].speed += gravity;
			particles[i].speed *= airdrag;
			particles[i].life += particles[i].decayrate;
			particles[i].size = particles[i].size * sizeMod + sizeGrowth;

			deleteMe = false;
		}
	}
}

void CSimpleParticleSystem::Init(const CUnit* owner, const float3& offset)
{
	CProjectile::Init(owner, offset);

	particles.resize(numParticles);

	const float3 up = emitVector;
	const float3 right = up.cross(float3(up.y, up.z, -up.x));
	const float3 forward = up.cross(right);

	// FIXME: should catch these earlier and for more projectile-types
	if (colorMap == NULL) {
		colorMap = CColorMap::LoadFromFloatVector(std::vector<float>(8, 1.0f));
		LOG_L(L_WARNING, "[CSimpleParticleSystem::%s] no color-map specified", __FUNCTION__);
	}
	if (texture == NULL) {
		texture = &projectileDrawer->textureAtlas->GetTexture("simpleparticle");
		LOG_L(L_WARNING, "[CSimpleParticleSystem::%s] no texture specified", __FUNCTION__);
	}

	for (int i = 0; i < numParticles; i++) {
		float az = gu->RandFloat() * 2 * PI;
		float ay = (emitRot + (emitRotSpread * gu->RandFloat())) * (PI / 180.0);

		particles[i].pos = offset;
		particles[i].speed = ((up * emitMul.y) * math::cos(ay) - ((right * emitMul.x) * math::cos(az) - (forward * emitMul.z) * math::sin(az)) * math::sin(ay)) * (particleSpeed + (gu->RandFloat() * particleSpeedSpread));
		particles[i].life = 0;
		particles[i].decayrate = 1.0f / (particleLife + (gu->RandFloat() * particleLifeSpread));
		particles[i].size = particleSize + gu->RandFloat()*particleSizeSpread;
	}

	drawRadius = (particleSpeed + particleSpeedSpread) * (particleLife * particleLifeSpread);
}






CR_BIND_DERIVED(CSphereParticleSpawner, CSimpleParticleSystem, );

CR_REG_METADATA(CSphereParticleSpawner,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
	CR_MEMBER_ENDFLAG(CM_Config)
));

CSphereParticleSpawner::CSphereParticleSpawner(): CSimpleParticleSystem()
{
}


void CSphereParticleSpawner::Init(CUnit* owner, const float3& offset)
{
	const float3 up = emitVector;
	const float3 right = up.cross(float3(up.y, up.z, -up.x));
	const float3 forward = up.cross(right);

	// FIXME: should catch these earlier and for more projectile-types
	if (colorMap == NULL) {
		colorMap = CColorMap::LoadFromFloatVector(std::vector<float>(8, 1.0f));
		LOG_L(L_WARNING, "[CSphereParticleSpawner::%s] no color-map specified", __FUNCTION__);
	}
	if (texture == NULL) {
		texture = &projectileDrawer->textureAtlas->GetTexture("sphereparticle");
		LOG_L(L_WARNING, "[CSphereParticleSpawner::%s] no texture specified", __FUNCTION__);
	}

	for (int i = 0; i < numParticles; i++) {
		const float az = gu->RandFloat() * 2 * PI;
		const float ay = (emitRot + emitRotSpread*gu->RandFloat()) * (PI / 180.0);

		const float3 pspeed = ((up * emitMul.y) * math::cos(ay) - ((right * emitMul.x) * math::cos(az) - (forward * emitMul.z) * math::sin(az)) * math::sin(ay)) * (particleSpeed + (gu->RandFloat() * particleSpeedSpread));

		CGenericParticleProjectile* particle = new CGenericParticleProjectile(owner, pos + offset, pspeed);

		particle->decayrate = 1.0f / (particleLife + gu->RandFloat() * particleLifeSpread);
		particle->life = 0;
		particle->size = particleSize + gu->RandFloat() * particleSizeSpread;

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
