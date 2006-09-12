#include "StdAfx.h"
#include "BeamLaserProjectile.h"
#include "Sim/Units/Unit.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "mmgr.h"
#include "ProjectileHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"

CBeamLaserProjectile::CBeamLaserProjectile(const float3& startPos,const float3& endPos,float startAlpha,float endAlpha,const float3& color, const float3& color2,CUnit* owner,float thickness, float corethickness, float flaresize, WeaponDef *weaponDef)
:	CWeaponProjectile((startPos+endPos)*0.5f,ZeroVector, owner, 0, ZeroVector, weaponDef,damages,0), //CProjectile((startPos+endPos)*0.5f,ZeroVector,owner),
	startPos(startPos),
	endPos(endPos),
	thickness(thickness),
	corethickness(corethickness),
	flaresize(flaresize)
{
	checkCol=false;
	useAirLos=true;

	SetRadius(pos.distance(endPos));

	midtexx = weaponDef->visuals.texture2->xstart + (weaponDef->visuals.texture2->xend-weaponDef->visuals.texture2->xstart)*0.5f;
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
		va->AddVertexTC(pos1-dir1*size,	midtexx,weaponDef->visuals.texture2->ystart,    kocolstart);
		va->AddVertexTC(pos1+dir1*size,	midtexx,weaponDef->visuals.texture2->yend,kocolstart);
		va->AddVertexTC(pos1+dir1*size-dir2*size, weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->yend,kocolstart);
		va->AddVertexTC(pos1-dir1*size-dir2*size, weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->ystart,kocolstart);
		va->AddVertexTC(pos1-dir1*coresize,midtexx,weaponDef->visuals.texture2->ystart,    corecolstart);
		va->AddVertexTC(pos1+dir1*coresize,midtexx,weaponDef->visuals.texture2->yend,corecolstart);
		va->AddVertexTC(pos1+dir1*coresize-dir2*coresize,weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->yend,corecolstart);
		va->AddVertexTC(pos1-dir1*coresize-dir2*coresize,weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->ystart,corecolstart);

		va->AddVertexTC(pos1-dir1*size,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->ystart,		kocolstart);
		va->AddVertexTC(pos1+dir1*size,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->yend,			kocolstart);
		va->AddVertexTC(pos2+dir1*size,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->yend,			kocolend);
		va->AddVertexTC(pos2-dir1*size,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->ystart,			kocolend);
		va->AddVertexTC(pos1-dir1*coresize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->ystart,	corecolstart);
		va->AddVertexTC(pos1+dir1*coresize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->yend,	corecolstart);
		va->AddVertexTC(pos2+dir1*coresize,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->yend,		corecolend);
		va->AddVertexTC(pos2-dir1*coresize,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->ystart,		corecolend);

		va->AddVertexTC(pos2-dir1*size,	midtexx,weaponDef->visuals.texture2->ystart,    kocolstart);
		va->AddVertexTC(pos2+dir1*size,	midtexx,weaponDef->visuals.texture2->yend,kocolstart);
		va->AddVertexTC(pos2+dir1*size+dir2*size, weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->yend,kocolstart);
		va->AddVertexTC(pos2-dir1*size+dir2*size, weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->ystart,kocolstart);
		va->AddVertexTC(pos2-dir1*coresize,midtexx,weaponDef->visuals.texture2->ystart,    corecolstart);
		va->AddVertexTC(pos2+dir1*coresize,midtexx,weaponDef->visuals.texture2->yend,corecolstart);
		va->AddVertexTC(pos2+dir1*coresize+dir2*coresize,weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->yend,corecolstart);
		va->AddVertexTC(pos2-dir1*coresize+dir2*coresize,weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->ystart,corecolstart);
	} else {
		va->AddVertexTC(pos1-dir1*size,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->ystart,		kocolstart);
		va->AddVertexTC(pos1+dir1*size,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->yend,			kocolstart);
		va->AddVertexTC(pos2+dir1*size,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->yend,			kocolend);
		va->AddVertexTC(pos2-dir1*size,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->ystart,			kocolend);
		va->AddVertexTC(pos1-dir1*coresize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->ystart,	corecolstart);
		va->AddVertexTC(pos1+dir1*coresize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->yend,	corecolstart);
		va->AddVertexTC(pos2+dir1*coresize,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->yend,		corecolend);
		va->AddVertexTC(pos2-dir1*coresize,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->ystart,		corecolend);
	}

	//draw flare
	float fsize = size*flaresize;
	va->AddVertexTC(pos1-camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture3->xstart,weaponDef->visuals.texture3->ystart,kocolstart);
	va->AddVertexTC(pos1+camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture3->xend,weaponDef->visuals.texture3->ystart,kocolstart);
	va->AddVertexTC(pos1+camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture3->xend,weaponDef->visuals.texture3->yend,kocolstart);
	va->AddVertexTC(pos1-camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture3->xstart,weaponDef->visuals.texture3->yend,kocolstart);

	fsize = fsize*corethickness;
	va->AddVertexTC(pos1-camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture3->xstart,weaponDef->visuals.texture3->ystart,corecolstart);
	va->AddVertexTC(pos1+camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture3->xend,weaponDef->visuals.texture3->ystart,corecolstart);
	va->AddVertexTC(pos1+camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture3->xend,weaponDef->visuals.texture3->yend,corecolstart);
	va->AddVertexTC(pos1-camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture3->xstart,weaponDef->visuals.texture3->yend,corecolstart);
}
