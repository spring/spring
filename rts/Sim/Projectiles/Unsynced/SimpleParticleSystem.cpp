#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "Sim/Misc/GlobalConstants.h"
#include "GenericParticleProjectile.h"
#include "GlobalUnsynced.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/ColorMap.h"
#include "SimpleParticleSystem.h"
#include "Sim/Projectiles/ProjectileHandler.h"

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

CSimpleParticleSystem::CSimpleParticleSystem(void)
:	CProjectile()
{
	checkCol=false;
	useAirLos=true;
	particles=0;
	emitMul = float3(1,1,1);
}

CSimpleParticleSystem::~CSimpleParticleSystem(void)
{
	if(particles)
		delete [] particles;
}

void CSimpleParticleSystem::Draw()
{
	inArray=true;

	va->EnlargeArrays(numParticles*4,0,VA_SIZE_TC);
	if(directional)
	{
		for(int i=0; i<numParticles; i++)
		{
			if(particles[i].life<1.0f)
			{
				float3 dif(particles[i].pos-camera->pos);
				dif.ANormalize();
				float3 dir1(dif.cross(particles[i].speed));
				dir1.ANormalize();
				float3 dir2(dif.cross(dir1));

				unsigned char color[4];

				colorMap->GetColor(color, particles[i].life);
				float3 interPos=particles[i].pos+particles[i].speed*gu->timeOffset;
				float size = particles[i].size;
				va->AddVertexQTC(interPos-dir1*size-dir2*size,texture->xstart,texture->ystart,color);
				va->AddVertexQTC(interPos-dir1*size+dir2*size,texture->xend ,texture->ystart,color);
				va->AddVertexQTC(interPos+dir1*size+dir2*size,texture->xend ,texture->yend ,color);
				va->AddVertexQTC(interPos+dir1*size-dir2*size,texture->xstart,texture->yend ,color);
			}
		}
	}
	else
	{
		unsigned char color[4];
		for(int i=0; i<numParticles; i++)
		{
			Particle* p = &particles[i];
			if(p->life<1.0f)
			{
				colorMap->GetColor(color, p->life);
				float3 interPos=p->pos+p->speed*gu->timeOffset;
				const float3 cameraRight = camera->right*p->size;
				const float3 cameraUp    = camera->up*p->size;

				va->AddVertexQTC(interPos-cameraRight-cameraUp,texture->xstart,texture->ystart,color);
				va->AddVertexQTC(interPos+cameraRight-cameraUp,texture->xend ,texture->ystart,color);
				va->AddVertexQTC(interPos+cameraRight+cameraUp,texture->xend ,texture->yend ,color);
				va->AddVertexQTC(interPos-cameraRight+cameraUp,texture->xstart,texture->yend ,color);
			}
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
			particles[i].pos += particles[i].speed;
			particles[i].life += particles[i].decayrate;
			particles[i].speed += gravity;

			particles[i].speed *= airdrag;

			particles[i].size = particles[i].size*sizeMod + sizeGrowth;

			deleteMe = false;
		}
	}

}

void CSimpleParticleSystem::Init(const float3& explosionPos, CUnit *owner GML_PARG_C)
{
	CProjectile::Init(explosionPos, owner GML_PARG_P);

	particles = new Particle[numParticles];

	float3 up = emitVector;
	float3 right = up.cross(float3(up.y,up.z,-up.x));
	float3 forward = up.cross(right);

	for(int i=0; i<numParticles; i++)
	{

		float az = gu->usRandFloat()*2*PI;
		float ay = (emitRot + emitRotSpread*gu->usRandFloat())*(PI/180.0);

		particles[i].decayrate = 1.0f/(particleLife + gu->usRandFloat()*particleLifeSpread);
		particles[i].life = 0;
		particles[i].size = particleSize + gu->usRandFloat()*particleSizeSpread;
		particles[i].pos = pos;

		particles[i].speed = ((up*emitMul.y)*cos(ay)-((right*emitMul.x)*cos(az)-(forward*emitMul.z)*sin(az))*sin(ay)) * (particleSpeed + gu->usRandFloat()*particleSpeedSpread);
	}

	drawRadius = (particleSpeed+particleSpeedSpread)*(particleLife*particleLifeSpread);
}


CR_BIND_DERIVED(CSphereParticleSpawner, CSimpleParticleSystem, );

CR_REG_METADATA(CSphereParticleSpawner,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
	CR_MEMBER_ENDFLAG(CM_Config)
));

CSphereParticleSpawner::CSphereParticleSpawner()
: 	CSimpleParticleSystem()
{
}

CSphereParticleSpawner::~CSphereParticleSpawner()
{
}

void CSphereParticleSpawner::Init(const float3& explosionPos, CUnit *owner GML_PARG_C)
{
	float3 up = emitVector;
	float3 right = up.cross(float3(up.y,up.z,-up.x));
	float3 forward = up.cross(right);

	for(int i=0; i<numParticles; i++)
	{

		float az = gu->usRandFloat()*2*PI;
		float ay = (emitRot + emitRotSpread*gu->usRandFloat())*(PI/180.0);

		float3 pspeed = ((up*emitMul.y)*cos(ay)-((right*emitMul.x)*cos(az)-(forward*emitMul.z)*sin(az))*sin(ay)) * (particleSpeed + gu->usRandFloat()*particleSpeedSpread);

		CGenericParticleProjectile *particle = new CGenericParticleProjectile(pos+explosionPos, pspeed, owner);

		particle->decayrate = 1.0f/(particleLife + gu->usRandFloat()*particleLifeSpread);
		particle->life = 0;
		particle->size = particleSize + gu->usRandFloat()*particleSizeSpread;

		particle->texture = texture;
		particle->colorMap = colorMap;

		particle->airdrag = airdrag;
		particle->sizeGrowth = sizeGrowth;
		particle->sizeMod = sizeMod;

		particle->gravity = gravity;
		particle->directional = directional;
		particle->SetRadius(particle->size + sizeGrowth*particleLife);
	}

	deleteMe = true;
}
