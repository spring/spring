#include "StdAfx.h"
#include "mmgr.h"

#include "FlareProjectile.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/Camera.h"
#include "LogOutput.h"
#include "ProjectileHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "WeaponProjectiles/MissileProjectile.h"
#include "GlobalUnsynced.h"

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

CFlareProjectile::CFlareProjectile(const float3& pos, const float3& speed, CUnit* owner, int activateFrame GML_PARG_C):
	//! these are synced, but neither weapon nor piece
	//! (only created by units that can drop flares)
	CProjectile(pos, speed, owner, true, false, false GML_PARG_P),
	activateFrame(activateFrame),
	deathFrame(activateFrame + (owner? owner->unitDef->flareTime: 1)),
	numSub(0),
	lastSub(0)
{
	alphaFalloff = owner? 1.0f / owner->unitDef->flareTime: 1.0f;
	checkCol = false;
	useAirLos = true;
	SetRadius(45);
	subPos.resize(owner->unitDef->flareSalvoSize);
	subSpeed.resize(owner->unitDef->flareSalvoSize);
}

CFlareProjectile::~CFlareProjectile(void)
{
}

void CFlareProjectile::Update(void)
{
	CUnit* owner = CProjectile::owner();
	if (gs->frameNum == activateFrame) {
		if (owner) {
			pos    = owner->pos;
			speed  = owner->speed;
			speed += owner->rightdir * owner->unitDef->flareDropVector.x;
			speed += owner->updir    * owner->unitDef->flareDropVector.y;
			speed += owner->frontdir * owner->unitDef->flareDropVector.z;
		} else {
			deleteMe = true;
		}
	}
	if (gs->frameNum >= activateFrame) {
		pos += speed;
		speed *= 0.95f;
		speed.y += gravity * 0.3f;

		//FIXME: just spawn new flares, if new missiles incoming?
		if(owner && lastSub<(gs->frameNum - owner->unitDef->flareSalvoDelay) && numSub<owner->unitDef->flareSalvoSize){
			subPos[numSub] = owner->pos;
			float3 s=owner->speed;
			s+=owner->rightdir*owner->unitDef->flareDropVector.x;
			s+=owner->updir*owner->unitDef->flareDropVector.y;
			s+=owner->frontdir*owner->unitDef->flareDropVector.z;
			subSpeed[numSub] = s;
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
		for (int a = 0; a < numSub; ++a) {
			subPos[a] += subSpeed[a];
			subSpeed[a] *= 0.95f;
			subSpeed[a].y += gravity * 0.3f;
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
	float alpha=std::max(0.0f,1-(gs->frameNum-activateFrame)*alphaFalloff);
	col[0]=(unsigned char)(alpha*255);
	col[1]=(unsigned char)(alpha*0.5f)*255;
	col[2]=(unsigned char)(alpha*0.2f)*255;
	col[3]=1;

	float rad=6.0;
	va->EnlargeArrays(numSub*4,0,VA_SIZE_TC);
	for(int a=0;a<numSub;++a){ //! CAUTION: loop count must match EnlargeArrays above
		float3 interPos=subPos[a]+subSpeed[a]*gu->timeOffset;

		va->AddVertexQTC(interPos-camera->right*rad-camera->up*rad,ph->flareprojectiletex.xstart,ph->flareprojectiletex.ystart,col);
		va->AddVertexQTC(interPos+camera->right*rad-camera->up*rad,ph->flareprojectiletex.xend,ph->flareprojectiletex.ystart,col);
		va->AddVertexQTC(interPos+camera->right*rad+camera->up*rad,ph->flareprojectiletex.xend,ph->flareprojectiletex.yend,col);
		va->AddVertexQTC(interPos-camera->right*rad+camera->up*rad,ph->flareprojectiletex.xstart,ph->flareprojectiletex.yend,col);
	}
}
