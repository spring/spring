#include "StdAfx.h"
#include "FlareProjectile.h"
#include "Game/Camera.h"
#include "LogOutput.h"
#include "Map/MapInfo.h"
#include "ProjectileHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "WeaponProjectiles/MissileProjectile.h"
#include "mmgr.h"

CR_BIND_DERIVED(CFlareProjectile, CProjectile, (float3(0,0,0),float3(0,0,0),0,0));

CR_REG_METADATA(CFlareProjectile,(
				CR_MEMBER(activateFrame),
				CR_MEMBER(deathFrame),

				CR_MEMBER(numSub),
				CR_MEMBER(lastSub),
				CR_MEMBER(subPos),
				CR_MEMBER(subSpeed),
				CR_MEMBER(alphaFalloff),
				CR_RESERVED(8)
				));

CFlareProjectile::CFlareProjectile(const float3& pos,const float3& speed,CUnit* owner,int activateFrame)
:	CProjectile(pos,speed,owner, true),
	activateFrame(activateFrame),
	deathFrame(activateFrame+(owner?owner->unitDef->flareTime:1)),
	numSub(0),
	lastSub(0)
{
	alphaFalloff=owner?1.0f/owner->unitDef->flareTime:1.0;
	checkCol=false;
	useAirLos=true;
	SetRadius(45);
}

CFlareProjectile::~CFlareProjectile(void)
{
}

void CFlareProjectile::Update(void)
{
	if(gs->frameNum==activateFrame){
		if(owner){
			pos=owner->pos;
			speed=owner->speed;
			speed+=owner->rightdir*owner->unitDef->flareDropVector.x;
			speed+=owner->updir*owner->unitDef->flareDropVector.y;
			speed+=owner->frontdir*owner->unitDef->flareDropVector.z;
		} else {
			deleteMe=true;
		}
	}
	if(gs->frameNum>=activateFrame){
		pos+=speed;
		speed*=0.95f;
		speed.y+=mapInfo->map.gravity*0.3f;

		if(owner && lastSub<gs->frameNum-owner->unitDef->flareSalvoDelay && numSub<owner->unitDef->flareSalvoSize){
			subPos.push_back(owner->pos);
			float3 s=owner->speed;
			s+=owner->rightdir*owner->unitDef->flareDropVector.x;
			s+=owner->updir*owner->unitDef->flareDropVector.y;
			s+=owner->frontdir*owner->unitDef->flareDropVector.z;
			subSpeed.push_back(s);
			++numSub;
			lastSub=gs->frameNum;

			for(std::list<CMissileProjectile*>::iterator mi=owner->incomingMissiles.begin();mi!=owner->incomingMissiles.end();++mi){
				if(gs->randFloat()<owner->unitDef->flareEfficiency){
					CMissileProjectile* missile=*mi;
					missile->decoyTarget=this;
					missile->AddDeathDependence(this);
				}
			}
		}
		for(int a=0;a<numSub;++a){
			subPos[a]+=subSpeed[a];
			subSpeed[a]*=0.95f;
			subSpeed[a].y+=mapInfo->map.gravity*0.3f;
		}
	}

	if(gs->frameNum>=deathFrame)
		deleteMe=true;
}

void CFlareProjectile::Draw(void)
{
	if(gs->frameNum<=activateFrame)
		return;

	inArray=true;
	unsigned char col[4];
	float alpha=max(0.0f,1-(gs->frameNum-activateFrame)*alphaFalloff);
	col[0]=(unsigned char)alpha*255;
	col[1]=(unsigned char)(alpha*0.5f)*255;
	col[2]=(unsigned char)(alpha*0.2f)*255;
	col[3]=1;

	for(int a=0;a<numSub;++a){
		float3 interPos=subPos[a]+subSpeed[a]*gu->timeOffset;
		float rad=5;

		va->AddVertexTC(interPos-camera->right*rad-camera->up*rad,ph->flareprojectiletex.xstart,ph->flareprojectiletex.ystart,col);
		va->AddVertexTC(interPos+camera->right*rad-camera->up*rad,ph->flareprojectiletex.xend,ph->flareprojectiletex.ystart,col);
		va->AddVertexTC(interPos+camera->right*rad+camera->up*rad,ph->flareprojectiletex.xend,ph->flareprojectiletex.yend,col);
		va->AddVertexTC(interPos-camera->right*rad+camera->up*rad,ph->flareprojectiletex.xstart,ph->flareprojectiletex.yend,col);
	}
}
