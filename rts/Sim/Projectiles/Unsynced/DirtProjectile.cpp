#include "StdAfx.h"
// DirtProjectile.cpp: implementation of the CDirtCloudProjectile class.
//
//////////////////////////////////////////////////////////////////////
#include "mmgr.h"

#include "DirtProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CDirtProjectile, CProjectile, );

CR_REG_METADATA(CDirtProjectile,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(alpha),
		CR_MEMBER(alphaFalloff),
		CR_MEMBER(size),
		CR_MEMBER(sizeExpansion),
		CR_MEMBER(slowdown),
		CR_MEMBER(color),
		CR_MEMBER(texture),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_RESERVED(8)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDirtProjectile::CDirtProjectile(const float3 pos, const float3 speed, const float ttl, const float size, const float expansion, float slowdown, CUnit* owner, const float3& color GML_PARG_C):
	CProjectile(pos, speed, owner, false, false, false GML_PARG_P),
	alpha(255),
	size(size),
	sizeExpansion(expansion),
	slowdown(slowdown),
	color(color)
{
	checkCol = false;
	alphaFalloff = 255 / ttl;
	texture = &ph->randdotstex;
}

CDirtProjectile::CDirtProjectile() :
	CProjectile(),
	alpha(255.0f),
	alphaFalloff(10.0f),
	size(10.0f),
	sizeExpansion(0.0f),
	slowdown(1.0f)
{
	checkCol = false;
	texture = &ph->randdotstex;
}

CDirtProjectile::~CDirtProjectile()
{}

void CDirtProjectile::Update()
{
	speed *= slowdown;
	speed.y += gravity;
	pos += speed;
	alpha -= alphaFalloff;
	size += sizeExpansion;

	if (ground->GetApproximateHeight(pos.x, pos.z) - 40 > pos.y) {
		deleteMe = true;
	}
	if (alpha <= 0) {
		deleteMe = true;
		alpha = 0;
	}
}

void CDirtProjectile::Draw()
{
	float partAbove=(pos.y/(size*camera->up.y));
	if(partAbove<-1)
		return;
	else if(partAbove>1)
		partAbove=1;
	inArray=true;
	unsigned char col[4];
	col[0]=(unsigned char) (color.x*alpha);
	col[1]=(unsigned char) (color.y*alpha);
	col[2]=(unsigned char) (color.z*alpha);
	col[3]=(unsigned char) (alpha)/*-gu->timeOffset*alphaFalloff*/;

	float interSize=size+gu->timeOffset*sizeExpansion;
	float texx = texture->xstart + (texture->xend-texture->xstart)*((1-partAbove)*0.5f);//0.25f*(1-partAbove)

	va->AddVertexTC(drawPos-camera->right*interSize-camera->up*interSize*partAbove,texx,texture->ystart,col);
	va->AddVertexTC(drawPos+camera->right*interSize-camera->up*interSize*partAbove,texx,texture->yend,col);
	va->AddVertexTC(drawPos+camera->right*interSize+camera->up*interSize,texture->xend,texture->yend,col);
	va->AddVertexTC(drawPos-camera->right*interSize+camera->up*interSize,texture->xend,texture->ystart,col);
}
