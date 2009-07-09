#include "StdAfx.h"
#include "mmgr.h"

#include "ExploSpikeProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CExploSpikeProjectile, CProjectile, );

CR_REG_METADATA(CExploSpikeProjectile,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(length),
		CR_MEMBER(width),
		CR_MEMBER(alpha),
		CR_MEMBER(alphaDecay),
		CR_MEMBER(lengthGrowth),
		CR_MEMBER(dir),
		CR_MEMBER(color),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_RESERVED(8)
));

CExploSpikeProjectile::CExploSpikeProjectile()
{
}

CExploSpikeProjectile::CExploSpikeProjectile(const float3& pos, const float3& speed, float length, float width, float alpha, float alphaDecay, CUnit* owner GML_PARG_C):
	CProjectile(pos, speed, owner, false, false, false GML_PARG_P),
	length(length),
	width(width),
	alpha(alpha),
	alphaDecay(alphaDecay),
	dir(speed),
	color(1.0f, 0.8f, 0.5f)
{
	lengthGrowth = dir.Length() * (0.5f + gu->usRandFloat() * 0.4f);
	dir /= lengthGrowth;

	checkCol  = false;
	useAirLos = true;
	SetRadius(length + lengthGrowth * alpha / alphaDecay);
}

CExploSpikeProjectile::~CExploSpikeProjectile(void)
{
}

void CExploSpikeProjectile::Update(void)
{
	pos+=speed;
	length+=lengthGrowth;
	alpha-=alphaDecay;

	if(alpha<=0){
		alpha=0;
		deleteMe=true;
	}
}

void CExploSpikeProjectile::Draw(void)
{
	inArray=true;

	float3 dif(pos-camera->pos2);
	dif.ANormalize();
	float3 dir2(dif.cross(dir));
	dir2.ANormalize();

	unsigned char col[4];
	float a=std::max(0.f,alpha-alphaDecay*gu->timeOffset)*255;
	col[0]=(unsigned char)(a*color.x);
	col[1]=(unsigned char)(a*color.y);
	col[2]=(unsigned char)(a*color.z);
	col[3]=1;

	float3 l=dir*length+lengthGrowth*gu->timeOffset;
	float3 w=dir2*width;

	va->AddVertexTC(drawPos+l+w, ph->laserendtex.xend, ph->laserendtex.yend, col);
	va->AddVertexTC(drawPos+l-w, ph->laserendtex.xend, ph->laserendtex.ystart, col);
	va->AddVertexTC(drawPos-l-w, ph->laserendtex.xstart, ph->laserendtex.ystart, col);
	va->AddVertexTC(drawPos-l+w, ph->laserendtex.xstart, ph->laserendtex.yend, col);
}

void CExploSpikeProjectile::Init(const float3& pos, CUnit *owner GML_PARG_C)
{
	CProjectile::Init(pos, owner GML_PARG_P);

	lengthGrowth=dir.Length()*(0.5f+gu->usRandFloat()*0.4f);
	dir/=lengthGrowth;

	checkCol=false;
	useAirLos=true;
	SetRadius(length+lengthGrowth*alpha/alphaDecay);
}
