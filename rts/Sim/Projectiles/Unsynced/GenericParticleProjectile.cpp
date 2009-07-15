#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "GenericParticleProjectile.h"
#include "GlobalUnsynced.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/ColorMap.h"
#include "Sim/Projectiles/ProjectileHandler.h"

CR_BIND_DERIVED(CGenericParticleProjectile, CProjectile, (float3(0,0,0),float3(0,0,0),NULL));

CR_REG_METADATA(CGenericParticleProjectile,(
	CR_MEMBER(gravity3),
	CR_MEMBER(texture),
	CR_MEMBER(colorMap),
	CR_MEMBER(directional),
	CR_MEMBER(life),
	CR_MEMBER(decayrate),
	CR_MEMBER(size),
	CR_MEMBER(airdrag),
	CR_MEMBER(sizeGrowth),
	CR_MEMBER(sizeMod),
	CR_RESERVED(8)
	));

CGenericParticleProjectile::CGenericParticleProjectile(const float3& pos, const float3& speed, CUnit* owner GML_PARG_C) :
	CProjectile(pos, speed, owner, false, false, false GML_PARG_P)
{
	deleteMe  = false;
	checkCol  = false;
	useAirLos = true;
}

CGenericParticleProjectile::~CGenericParticleProjectile(void)
{
}

void CGenericParticleProjectile::Update()
{
	pos += speed;
	life +=decayrate;
	speed += gravity3;
	speed *= airdrag;
	size = size * sizeMod + sizeGrowth;

	if (life > 1.0f) {
		deleteMe = true;
	}
}

void CGenericParticleProjectile::Draw()
{
	inArray=true;

	if(directional)
	{
		float3 dif(pos-camera->pos);
		dif.ANormalize();
		float3 dir1(dif.cross(speed));
		dir1.ANormalize();
		float3 dir2(dif.cross(dir1));

		unsigned char color[4];

		colorMap->GetColor(color, life);

		va->AddVertexTC(drawPos-dir1*size-dir2*size,texture->xstart,texture->ystart,color);
		va->AddVertexTC(drawPos-dir1*size+dir2*size,texture->xend ,texture->ystart,color);
		va->AddVertexTC(drawPos+dir1*size+dir2*size,texture->xend ,texture->yend ,color);
		va->AddVertexTC(drawPos+dir1*size-dir2*size,texture->xstart,texture->yend ,color);
	}
	else
	{
		unsigned char color[4];

		colorMap->GetColor(color, life);

		va->AddVertexTC(drawPos-camera->right*size-camera->up*size,texture->xstart,texture->ystart,color);
		va->AddVertexTC(drawPos+camera->right*size-camera->up*size,texture->xend ,texture->ystart,color);
		va->AddVertexTC(drawPos+camera->right*size+camera->up*size,texture->xend ,texture->yend ,color);
		va->AddVertexTC(drawPos-camera->right*size+camera->up*size,texture->xstart,texture->yend ,color);
	}
}
