#include "StdAfx.h"
#include "BeamLaserProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "mmgr.h"

CR_BIND_DERIVED(CBeamLaserProjectile, CWeaponProjectile,
		(float3(0,0,0),float3(0,0,0),0,0,float3(0,0,0),float3(0,0,0),NULL,0,0,0,NULL,0,0));

CR_REG_METADATA(CBeamLaserProjectile,(
	CR_MEMBER(startPos),
	CR_MEMBER(endPos),
	CR_MEMBER(corecolstart),
	CR_MEMBER(corecolend),
	CR_MEMBER(kocolstart),
	CR_MEMBER(kocolend),
	CR_MEMBER(thickness),
	CR_MEMBER(corethickness),
	CR_MEMBER(flaresize),
	CR_MEMBER(decay),
	CR_MEMBER(midtexx),
	CR_RESERVED(16)
	));

CBeamLaserProjectile::CBeamLaserProjectile(const float3& startPos, const float3& endPos,
	float startAlpha, float endAlpha, const float3& color, const float3& color2,
	CUnit* owner, float thickness, float corethickness, float flaresize,
	const WeaponDef* weaponDef, int ttl, float decay):

	CWeaponProjectile((startPos + endPos) * 0.5f, ZeroVector, owner, 0, ZeroVector, weaponDef, 0, false,  ttl),
	startPos(startPos),
	endPos(endPos),
	thickness(thickness),
	corethickness(corethickness),
	flaresize(flaresize),
	decay(decay)
{
	checkCol=false;
	useAirLos=true;

	SetRadius(pos.distance(endPos));

	if (weaponDef) {
		midtexx = weaponDef->visuals.texture2->xstart
				+ (weaponDef->visuals.texture2->xend
						- weaponDef->visuals.texture2->xstart) * 0.5f;
	} else {
		midtexx=0;
	}
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

	if (cegTag.size() > 0) {
		ceg.Load(explGenHandler, cegTag);
	}
}

CBeamLaserProjectile::~CBeamLaserProjectile(void)
{
}

void CBeamLaserProjectile::Update(void)
{
	if (ttl <= 0)
		deleteMe = true;
	else {
		ttl--;
		for (int i = 0; i < 3; i++) {
			corecolstart[i] = (unsigned char) (corecolstart[i] * decay);
			corecolend[i] = (unsigned char) (corecolend[i] * decay);
			kocolstart[i] = (unsigned char) (kocolstart[i] * decay);
			kocolend[i] = (unsigned char) (kocolend[i] * decay);
		}

		if (cegTag.size() > 0) {
			ceg.Explosion(startPos + ((endPos - startPos) / ttl), 0.0f, flaresize, 0x0, 0.0f, 0x0, endPos - startPos);
		}
	}
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
