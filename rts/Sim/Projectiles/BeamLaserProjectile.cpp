#include "StdAfx.h"
#include "BeamLaserProjectile.h"
#include "Sim/Units/Unit.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "mmgr.h"
#include "ProjectileHandler.h"

CBeamLaserProjectile::CBeamLaserProjectile(const float3& startPos,const float3& endPos,float startAlpha,float endAlpha,const float3& color, const float3& color2,CUnit* owner,float thickness, float corethickness, float flaresize)
: CProjectile((startPos+endPos)*0.5,ZeroVector,owner),
	startPos(startPos),
	endPos(endPos),
	thickness(thickness),
	corethickness(corethickness),
	flaresize(flaresize)
{
	checkCol=false;
	useAirLos=true;

	SetRadius(pos.distance(endPos));

	midtexx = ph->laserendtex.xstart + (ph->laserendtex.xend-ph->laserendtex.xstart)*0.5;
	corecolstart[0]=(unsigned char)(color2.x*startAlpha);
	corecolstart[1]=(unsigned char)(color2.y*startAlpha);
	corecolstart[2]=(unsigned char)(color2.z*startAlpha);
	corecolstart[3]=1;
	corecolend[0]=(unsigned char)(color2.x*endAlpha);
	corecolend[1]=(unsigned char)(color2.y*endAlpha);
	corecolend[2]=(unsigned char)(color2.z*endAlpha);
	corecolend[3]=1;
	kocolstart[0]=(unsigned char)(color.x*startAlpha);
	kocolstart[1]=(unsigned char)(color.y*startAlpha);
	kocolstart[2]=(unsigned char)(color.z*startAlpha);
	kocolstart[3]=1;
	kocolend[0]=(unsigned char)(color.x*endAlpha);
	kocolend[1]=(unsigned char)(color.y*endAlpha);
	kocolend[2]=(unsigned char)(color.z*endAlpha);
	kocolend[3]=1;
}

CBeamLaserProjectile::~CBeamLaserProjectile(void)
{
}

void CBeamLaserProjectile::Update(void)
{
	deleteMe=true;
}

void CBeamLaserProjectile::Draw(void)
{
	inArray=true;
	float3 dif(pos-camera->pos);
	float camDist=dif.Length();
	dif/=camDist;
	float3 dir=endPos-startPos;
	dir.Normalize();
	float3 dir1(dif.cross(dir));
	dir1.Normalize();
	float3 dir2(dif.cross(dir1));

	float size=thickness;
	float coresize=size*corethickness;
	float3 pos1=startPos;
	float3 pos2=endPos;

	if(camDist<1000){
		va->AddVertexTC(pos1-dir1*size,	midtexx,ph->laserendtex.ystart,    kocolstart);
		va->AddVertexTC(pos1+dir1*size,	midtexx,ph->laserendtex.yend,kocolstart);
		va->AddVertexTC(pos1+dir1*size-dir2*size, ph->laserendtex.xend,ph->laserendtex.yend,kocolstart);
		va->AddVertexTC(pos1-dir1*size-dir2*size, ph->laserendtex.xend,ph->laserendtex.ystart,kocolstart);
		va->AddVertexTC(pos1-dir1*coresize,midtexx,ph->laserendtex.ystart,    corecolstart);
		va->AddVertexTC(pos1+dir1*coresize,midtexx,ph->laserendtex.yend,corecolstart);
		va->AddVertexTC(pos1+dir1*coresize-dir2*coresize,ph->laserendtex.xend,ph->laserendtex.yend,corecolstart);
		va->AddVertexTC(pos1-dir1*coresize-dir2*coresize,ph->laserendtex.xend,ph->laserendtex.ystart,corecolstart);

		va->AddVertexTC(pos1-dir1*size,ph->laserfallofftex.xstart,ph->laserfallofftex.ystart,		kocolstart);
		va->AddVertexTC(pos1+dir1*size,ph->laserfallofftex.xstart,ph->laserfallofftex.yend,			kocolstart);
		va->AddVertexTC(pos2+dir1*size,ph->laserfallofftex.xend,ph->laserfallofftex.yend,			kocolend);
		va->AddVertexTC(pos2-dir1*size,ph->laserfallofftex.xend,ph->laserfallofftex.ystart,			kocolend);
		va->AddVertexTC(pos1-dir1*coresize,ph->laserfallofftex.xstart,ph->laserfallofftex.ystart,	corecolstart);
		va->AddVertexTC(pos1+dir1*coresize,ph->laserfallofftex.xstart,ph->laserfallofftex.yend,	corecolstart);
		va->AddVertexTC(pos2+dir1*coresize,ph->laserfallofftex.xend,ph->laserfallofftex.yend,		corecolend);
		va->AddVertexTC(pos2-dir1*coresize,ph->laserfallofftex.xend,ph->laserfallofftex.ystart,		corecolend);

		va->AddVertexTC(pos2-dir1*size,	midtexx,ph->laserendtex.ystart,    kocolstart);
		va->AddVertexTC(pos2+dir1*size,	midtexx,ph->laserendtex.yend,kocolstart);
		va->AddVertexTC(pos2+dir1*size+dir2*size, ph->laserendtex.xend,ph->laserendtex.yend,kocolstart);
		va->AddVertexTC(pos2-dir1*size+dir2*size, ph->laserendtex.xend,ph->laserendtex.ystart,kocolstart);
		va->AddVertexTC(pos2-dir1*coresize,midtexx,ph->laserendtex.ystart,    corecolstart);
		va->AddVertexTC(pos2+dir1*coresize,midtexx,ph->laserendtex.yend,corecolstart);
		va->AddVertexTC(pos2+dir1*coresize+dir2*coresize,ph->laserendtex.xend,ph->laserendtex.yend,corecolstart);
		va->AddVertexTC(pos2-dir1*coresize+dir2*coresize,ph->laserendtex.xend,ph->laserendtex.ystart,corecolstart);
	} else {
		va->AddVertexTC(pos1-dir1*size,ph->laserfallofftex.xstart,ph->laserfallofftex.ystart,		kocolstart);
		va->AddVertexTC(pos1+dir1*size,ph->laserfallofftex.xstart,ph->laserfallofftex.yend,			kocolstart);
		va->AddVertexTC(pos2+dir1*size,ph->laserfallofftex.xend,ph->laserfallofftex.yend,			kocolend);
		va->AddVertexTC(pos2-dir1*size,ph->laserfallofftex.xend,ph->laserfallofftex.ystart,			kocolend);
		va->AddVertexTC(pos1-dir1*coresize,ph->laserfallofftex.xstart,ph->laserfallofftex.ystart,	corecolstart);
		va->AddVertexTC(pos1+dir1*coresize,ph->laserfallofftex.xstart,ph->laserfallofftex.yend,	corecolstart);
		va->AddVertexTC(pos2+dir1*coresize,ph->laserfallofftex.xend,ph->laserfallofftex.yend,		corecolend);
		va->AddVertexTC(pos2-dir1*coresize,ph->laserfallofftex.xend,ph->laserfallofftex.ystart,		corecolend);
	}

	//draw flare
	float fsize = size*flaresize;
	va->AddVertexTC(pos1-camera->right*fsize-camera->up*fsize,ph->flaretex.xstart,ph->flaretex.ystart,kocolstart);
	va->AddVertexTC(pos1+camera->right*fsize-camera->up*fsize,ph->flaretex.xend,ph->flaretex.ystart,kocolstart);
	va->AddVertexTC(pos1+camera->right*fsize+camera->up*fsize,ph->flaretex.xend,ph->flaretex.yend,kocolstart);
	va->AddVertexTC(pos1-camera->right*fsize+camera->up*fsize,ph->flaretex.xstart,ph->flaretex.yend,kocolstart);

	fsize = fsize*corethickness;
	va->AddVertexTC(pos1-camera->right*fsize-camera->up*fsize,ph->flaretex.xstart,ph->flaretex.ystart,corecolstart);
	va->AddVertexTC(pos1+camera->right*fsize-camera->up*fsize,ph->flaretex.xend,ph->flaretex.ystart,corecolstart);
	va->AddVertexTC(pos1+camera->right*fsize+camera->up*fsize,ph->flaretex.xend,ph->flaretex.yend,corecolstart);
	va->AddVertexTC(pos1-camera->right*fsize+camera->up*fsize,ph->flaretex.xstart,ph->flaretex.yend,corecolstart);
}
