#include "StdAfx.h"
#include "FireProjectile.h"
#include "Sim/Misc/Wind.h"
#include "Rendering/GL/VertexArray.h"
#include "Game/Camera.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/Feature.h"
#include "ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "mmgr.h"
#include "ProjectileHandler.h"

CFireProjectile::CFireProjectile(const float3& pos,const float3& speed,CUnit* owner,int emitTtl,float emitRadius,int particleTtl,float particleSize)
: CProjectile(pos,speed,owner),
	ttl(emitTtl),
	emitPos(pos),
	emitRadius(emitRadius),
	particleTime(particleTtl),
	particleSize(particleSize)
{
	drawRadius=emitRadius+particleTime*speed.Length();
	checkCol=false;
	this->pos.y+=particleTime*speed.Length()*0.5f;
	ageSpeed=1.0f/particleTime;

	alwaysVisible=true;
	castShadow=true;
}

CFireProjectile::~CFireProjectile(void)
{
}

void CFireProjectile::StopFire(void)
{
	ttl=0;
}

void CFireProjectile::Update(void)
{
	ttl--;
	if(ttl>0){
		if(ph->particleSaturation<0.8f || (ph->particleSaturation<1 && (gs->frameNum & 1))){	//this area must be unsynced
			SubParticle sub;
			sub.age=0;
			sub.maxSize=(0.7f+gu->usRandFloat()*0.3f)*particleSize;
			sub.posDif=gu->usRandVector()*emitRadius;
			sub.pos=emitPos;
			sub.pos.y+=sub.posDif.y;
			sub.posDif.y=0;
			sub.rotSpeed=(gu->usRandFloat()-0.5f)*4;
			sub.smokeType=gu->usRandInt()%6;
			subParticles.push_front(sub);

			sub.maxSize=(0.7f+gu->usRandFloat()*0.3f)*particleSize;
			sub.posDif=gu->usRandVector()*emitRadius;
			sub.pos=emitPos;
			sub.pos.y+=sub.posDif.y-radius*0.3f;
			sub.posDif.y=0;
			sub.rotSpeed=(gu->usRandFloat()-0.5f)*4;
			subParticles2.push_front(sub);
		}
		if(!(ttl&31)){		//this area must be synced
			std::vector<CFeature*> f=qf->GetFeaturesExact(emitPos+wind.curWind*0.7f,emitRadius*2);
			for(std::vector<CFeature*>::iterator fi=f.begin();fi!=f.end();++fi){
				if(gs->randFloat()>0.8f)
					(*fi)->StartFire();
			}
			std::vector<CUnit*> units=qf->GetUnitsExact(emitPos+wind.curWind*0.7f,emitRadius*2);
			for(std::vector<CUnit*>::iterator ui=units.begin();ui!=units.end();++ui){
				(*ui)->DoDamage(DamageArray()*30,0,ZeroVector);
			}
		}
	}

	for(std::list<SubParticle>::iterator pi=subParticles.begin();pi!=subParticles.end();++pi){
		pi->age+=ageSpeed;
		if(pi->age>1){
			subParticles.pop_back();
			break;
		}
		pi->pos+=speed + wind.curWind*pi->age*0.05f + pi->posDif*0.1f;
		pi->posDif*=0.9f;
	}
	for(std::list<SubParticle>::iterator pi=subParticles2.begin();pi!=subParticles2.end();++pi){
		pi->age+=ageSpeed*1.5f;
		if(pi->age>1){
			subParticles2.pop_back();
			break;
		}
		pi->pos+=speed*0.7f+pi->posDif*0.1f;
		pi->posDif*=0.9f;
	}

	if(subParticles.empty() && ttl<=0)
		deleteMe=true;
}

void CFireProjectile::Draw(void)
{
	inArray=true;
	unsigned char col[4];
	col[3]=1;
	unsigned char col2[4];
	for(std::list<SubParticle>::iterator pi=subParticles2.begin();pi!=subParticles2.end();++pi){
		float age=pi->age+ageSpeed*gu->timeOffset;
		float size=pi->maxSize*(age);
		float rot=pi->rotSpeed*age;

		float sinRot=sin(rot);
		float cosRot=cos(rot);
		float3 dir1=(camera->right*cosRot+camera->up*sinRot)*size;
		float3 dir2=(camera->right*sinRot-camera->up*cosRot)*size;

		float3 interPos=pi->pos;

		col[0]=(unsigned char)((1-age)*255);
		col[1]=(unsigned char)((1-age)*255);
		col[2]=(unsigned char)((1-age)*255);

		va->AddVertexTC(interPos-dir1-dir2,ph->explotex.xstart,ph->explotex.ystart,col);
		va->AddVertexTC(interPos+dir1-dir2,ph->explotex.xend ,ph->explotex.ystart,col);
		va->AddVertexTC(interPos+dir1+dir2,ph->explotex.xend ,ph->explotex.yend ,col);
		va->AddVertexTC(interPos-dir1+dir2,ph->explotex.xstart,ph->explotex.yend ,col);
	}
	for(std::list<SubParticle>::iterator pi=subParticles.begin();pi!=subParticles.end();++pi){
		float age=pi->age+ageSpeed*gu->timeOffset;
		float size=pi->maxSize*sqrt(age);
		float rot=pi->rotSpeed*age;

		float sinRot=sin(rot);
		float cosRot=cos(rot);
		float3 dir1=(camera->right*cosRot+camera->up*sinRot)*size;
		float3 dir2=(camera->right*sinRot-camera->up*cosRot)*size;

		float3 interPos=pi->pos;

		if(age < 1/1.31f){
			col[0]=(unsigned char)((1-age*1.3f)*255);
			col[1]=(unsigned char)((1-age*1.3f)*255);
			col[2]=(unsigned char)((1-age*1.3f)*255);
			col[3]=1;

			va->AddVertexTC(interPos-dir1-dir2,ph->explotex.xstart,ph->explotex.ystart,col);
			va->AddVertexTC(interPos+dir1-dir2,ph->explotex.xend ,ph->explotex.ystart,col);
			va->AddVertexTC(interPos+dir1+dir2,ph->explotex.xend ,ph->explotex.yend ,col);
			va->AddVertexTC(interPos-dir1+dir2,ph->explotex.xstart,ph->explotex.yend ,col);
		}

		unsigned char c;
		if(age<0.5f){
			c=(unsigned char)(age*510);
		} else {
			c=(unsigned char)(510-age*510);
		}
		col2[0]=(unsigned char)(c*0.6f);
		col2[1]=(unsigned char)(c*0.6f);
		col2[2]=(unsigned char)(c*0.6f);
		col2[3]=c;
		float xmod=0.125f+(float(int(pi->smokeType%6)))/16;
		float ymod=(int(pi->smokeType/6))/16.0f;

		va->AddVertexTC(interPos-dir1-dir2,ph->smoketex[pi->smokeType].xstart,ph->smoketex[pi->smokeType].ystart,col2);
		va->AddVertexTC(interPos+dir1-dir2,ph->smoketex[pi->smokeType].xend,ph->smoketex[pi->smokeType].ystart,col2);
		va->AddVertexTC(interPos+dir1+dir2,ph->smoketex[pi->smokeType].xend,ph->smoketex[pi->smokeType].yend,col2);
		va->AddVertexTC(interPos-dir1+dir2,ph->smoketex[pi->smokeType].xstart,ph->smoketex[pi->smokeType].yend,col2);
	}
}

