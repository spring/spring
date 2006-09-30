#include "StdAfx.h"
#include "SimpleParticleSystem.h"
#include "GlobalStuff.h"
#include "Rendering/GL/VertexArray.h"
#include "Game/Camera.h"
#include "ProjectileHandler.h"
#include "Rendering/Textures/ColorMap.h"

CR_BIND_DERIVED(CSimpleParticleSystem, CProjectile);

CR_REG_METADATA(CSimpleParticleSystem, 
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(emitVector),
		CR_MEMBER(emitSpread),
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

	CR_MEMBER_ENDFLAG(CM_Config)
));

CSimpleParticleSystem::CSimpleParticleSystem(void)
{
	deleteMe=false;
	checkCol=false;
	useAirLos=true;
	particles=0;
}

CSimpleParticleSystem::~CSimpleParticleSystem(void)
{
	if(particles)
		delete [] particles;
}
	float3 fcol(1,1,1);
	float alpha = 0.01;


void CSimpleParticleSystem::Draw()
{
	inArray=true;


	for(int i=0; i<numParticles; i++)
	{
		if(particles[i].life<1.0f)
		{
			float3 dif(particles[i].pos-camera->pos);
			float camDist=dif.Length();
			dif/=camDist;
			float3 dir1(dif.cross(particles[i].speed));
			dir1.Normalize();
			float3 dir2(dif.cross(dir1));

			unsigned char color[4];

			colorMap->GetColor(color, particles[i].life);
			float3 interPos=particles[i].pos+particles[i].speed*gu->timeOffset;
			float size = particles[i].size;
			va->AddVertexTC(interPos-dir1*particleSize-dir2*particleSize,texture->xstart,texture->ystart,color);
			va->AddVertexTC(interPos-dir1*particleSize+dir2*particleSize,texture->xend ,texture->ystart,color);
			va->AddVertexTC(interPos+dir1*particleSize+dir2*particleSize,texture->xend ,texture->yend ,color);
			va->AddVertexTC(interPos+dir1*particleSize-dir2*particleSize,texture->xstart,texture->yend ,color);

			//va->AddVertexTC(interPos-camera->right*particleSize-camera->up*particleSize,texture->xstart,texture->ystart,color);
			//va->AddVertexTC(interPos+camera->right*particleSize-camera->up*particleSize,texture->xend ,texture->ystart,color);
			//va->AddVertexTC(interPos+camera->right*particleSize+camera->up*particleSize,texture->xend ,texture->yend ,color);
			//va->AddVertexTC(interPos-camera->right*particleSize+camera->up*particleSize,texture->xstart,texture->yend ,color);
		}
	}

}

void CSimpleParticleSystem::Update()
{
	deleteMe = true;
	for(int i=0; i<numParticles; i++)
	{
		if(particles[i].life<1.0f)
		{
			particles[i].life += particles[i].decayrate;
			particles[i].pos += particles[i].speed;
			particles[i].speed += gravity;
			particles[i].speed *= airdrag;
			deleteMe = false;
		}
	}

}

void CSimpleParticleSystem::Init(const float3& explosionPos, CUnit *owner)
{
	CProjectile::Init(explosionPos, owner);
	
	particles = new Particle[numParticles];

	for(int i=0; i<numParticles; i++)
	{
		particles[i].decayrate = 1.0f/(particleLife + gu->usRandFloat()*particleLifeSpread);
		particles[i].life = 0;
		particles[i].size = particleSize + gu->usRandFloat()*particleSizeSpread;
		particles[i].pos = pos;
		particles[i].speed = (emitVector*emitMul + emitSpread*gu->usRandVector()).Normalize() * (particleSpeed + particleSpeedSpread*(gu->usRandFloat()*2-1.0f));
	}
}
