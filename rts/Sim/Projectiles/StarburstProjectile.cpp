#include "StdAfx.h"
#include "StarburstProjectile.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Game/Camera.h"
#include "Sim/Units/Unit.h"
#include "SmokeTrailProjectile.h"
#include "Map/Ground.h"
#include "Game/GameHelper.h"
#include "myMath.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Matrix44f.h"
#include "Sync/SyncTracer.h"
#include "ProjectileHandler.h"
#include "mmgr.h"

#include "LogOutput.h"

static const float Smoke_Time=70;

CR_BIND_DERIVED(CStarburstProjectile, CWeaponProjectile, (float3(0,0,0),float3(0,0,0),NULL,float3(0,0,0),0,0,0,0,NULL,NULL,NULL,0));

CR_REG_METADATA(CStarburstProjectile,(
	CR_MEMBER(tracking),
	CR_MEMBER(maxGoodDif),
	CR_MEMBER(dir),
	CR_MEMBER(maxSpeed),
	CR_MEMBER(curSpeed),
	CR_MEMBER(ttl),
	CR_MEMBER(uptime),
	CR_MEMBER(areaOfEffect),
	CR_MEMBER(age),
	CR_MEMBER(oldSmoke),
	CR_MEMBER(oldSmokeDir),
	CR_MEMBER(drawTrail),
	CR_MEMBER(numParts),
	CR_MEMBER(doturn),
	CR_MEMBER(curCallback),
	CR_MEMBER(missileAge),
	CR_MEMBER(distanceToTravel),
	CR_RESERVED(16)
	));

void CStarburstProjectile::creg_Serialize(creg::ISerializer& s)
{
	s.Serialize(numCallback, sizeof(int));
	// NOTE This could be tricky if gs is serialized after losHandler.
	for(int a=0;a<5;++a){
		s.Serialize(oldInfos[a],sizeof(struct CStarburstProjectile::OldInfo));
	}
}

CStarburstProjectile::CStarburstProjectile(const float3& pos, const float3& speed, CUnit* owner, float3 targetPos, float areaOfEffect, float maxSpeed, float tracking, int uptime, CUnit* target, const WeaponDef* weaponDef, CWeaponProjectile* interceptTarget, float maxdistance)
: CWeaponProjectile(pos, speed, owner, target, targetPos, weaponDef, interceptTarget, true),
	ttl(200),
	maxSpeed(maxSpeed),
	tracking(tracking),
	dir(speed),
	oldSmoke(pos),
	age(0),
	drawTrail(true),
	numParts(0),
	doturn(true),
	curCallback(0),
	numCallback(0),
	missileAge(0),
	areaOfEffect(areaOfEffect),
	distanceToTravel(maxdistance)
{
	this->uptime=uptime;
	if (weaponDef->flighttime == 0) {
		ttl=(int)min(3000.f,uptime+(weaponDef?weaponDef->range:0)/maxSpeed+100);
	}
	else {
		ttl=weaponDef->flighttime;
	}

	maxGoodDif=cos(tracking*0.6f);
	curSpeed=speed.Length();
	dir.Normalize();
	oldSmokeDir=dir;

	drawRadius=maxSpeed*8;
	ENTER_MIXED;
	numCallback=SAFE_NEW int;
	*numCallback=0;
	float3 camDir=(pos-camera->pos).Normalize();
	if(camera->pos.distance(pos)*0.2f+(1-fabs(camDir.dot(dir)))*3000 < 200)
		drawTrail=false;
	ENTER_SYNCED;

	for(int a=0;a<5;++a){
		oldInfos[a]=SAFE_NEW OldInfo;
		oldInfos[a]->dir=dir;
		oldInfos[a]->pos=pos;
		oldInfos[a]->speedf=curSpeed;
	}
	castShadow=true;

#ifdef TRACE_SYNC
	tracefile << "New starburst rocket: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif

}

CStarburstProjectile::~CStarburstProjectile(void)
{
	delete numCallback;
	if(curCallback)
		curCallback->drawCallbacker=0;
	for(int a=0;a<5;++a){
		delete oldInfos[a];
	}
}

void CStarburstProjectile::Collision()
{
	float h=ground->GetHeight2(pos.x,pos.z);
	if(weaponDef->waterweapon && h < pos.y) return; //prevent impact on water if waterweapon is set
	if(h>pos.y)
		pos+=speed*(h-pos.y)/speed.y;
	if (weaponDef->visuals.smokeTrail)
		SAFE_NEW CSmokeTrailProjectile(pos,oldSmoke,dir,oldSmokeDir,owner,false,true,7,Smoke_Time,0.7f,drawTrail);
	oldSmokeDir=dir;
//	helper->Explosion(pos,damages,areaOfEffect,owner);
	CWeaponProjectile::Collision();
	oldSmoke=pos;
}

void CStarburstProjectile::Collision(CUnit *unit)
{
	if (weaponDef->visuals.smokeTrail)
		SAFE_NEW CSmokeTrailProjectile(pos,oldSmoke,dir,oldSmokeDir,owner,false,true,7,Smoke_Time,0.7f,drawTrail);
	oldSmokeDir=dir;
//	unit->DoDamage(damages,owner);
//	helper->Explosion(pos,damages,areaOfEffect,owner);

	CWeaponProjectile::Collision(unit);
	oldSmoke=pos;
}

void CStarburstProjectile::Update(void)
{
	ttl--;
	uptime--;
	missileAge++;
	if(target && weaponDef->tracks && owner){
		targetPos=helper->GetUnitErrorPos(target,owner->allyteam);
	}
	if(interceptTarget){
		targetPos=interceptTarget->pos;
		if(targetPos.distance(pos)<areaOfEffect*2){
			interceptTarget->Collision();
			Collision();
		}
	}
	if(uptime>0){
		if(curSpeed<maxSpeed)
			curSpeed+=weaponDef->weaponacceleration;
		speed=dir*curSpeed;
	} else if(doturn && ttl>0 && distanceToTravel>0) {
		float3 dif(targetPos-pos);
		dif.Normalize();
		if(dif.dot(dir)>0.99f){
			dir=dif;
			doturn=false;
		} else {
			dif=dif-dir;
			dif-=dir*(dif.dot(dir));
			dif.Normalize();
			if (weaponDef->turnrate != 0) {
				dir+=dif*weaponDef->turnrate;
			}
			else {
				dir+=dif*0.06;
			}
			dir.Normalize();
		}
		speed=dir*curSpeed;
		if (distanceToTravel != MAX_WORLD_SIZE)
			distanceToTravel-=speed.Length2D();
	} else if(ttl>0 && distanceToTravel>0) {
		if(curSpeed<maxSpeed)
			curSpeed+=weaponDef->weaponacceleration;
		float3 dif(targetPos-pos);
		dif.Normalize();
		if(dif.dot(dir)>maxGoodDif){
			dir=dif;
		} else {
			dif=dif-dir;
			dif-=dir*(dif.dot(dir));
			dif.Normalize();
			dir+=dif*tracking;
			dir.Normalize();
		}
		speed=dir*curSpeed;
		if (distanceToTravel != MAX_WORLD_SIZE)
			distanceToTravel-=speed.Length2D();
	} else {
		dir.y+=gs->gravity;
		dir.Normalize();
		curSpeed+=-gs->gravity;
		speed=dir*curSpeed;
	}
	pos+=speed;

	OldInfo* tempOldInfo=oldInfos[4];
	for(int a=3;a>=0;--a){
		oldInfos[a+1]=oldInfos[a];
	}
	oldInfos[0]=tempOldInfo;
	oldInfos[0]->pos=pos;
	oldInfos[0]->dir=dir;
	oldInfos[0]->speedf=curSpeed;
	oldInfos[0]->ageMods.clear();

	age++;
	numParts++;
	if(weaponDef->visuals.smokeTrail && !(age&7)){
		if(curCallback)
			curCallback->drawCallbacker=0;
		curCallback=SAFE_NEW CSmokeTrailProjectile(pos,oldSmoke,dir,oldSmokeDir,owner,age==8,false,7,Smoke_Time,0.7f,drawTrail,this);
		oldSmoke=pos;
		oldSmokeDir=dir;
		numParts=0;
		useAirLos=curCallback->useAirLos;
		if(!drawTrail){
			ENTER_MIXED;
			float3 camDir=(pos-camera->pos).Normalize();
			if(camera->pos.distance(pos)*0.2f+(1-fabs(camDir.dot(dir)))*3000 > 300)
				drawTrail=true;
			ENTER_SYNCED;
		}
	}
	*numCallback=0;
}

void CStarburstProjectile::Draw(void)
{
	float3 interPos=pos+speed*gu->timeOffset;
	inArray=true;
	float age2=(age&7)+gu->timeOffset;

	float color=0.7f;
	unsigned char col[4];
	unsigned char col2[4];

	if (weaponDef->visuals.smokeTrail)
		if(drawTrail){		//draw the trail as a single quad

			float3 dif(interPos-camera->pos);
			dif.Normalize();
			float3 dir1(dif.cross(dir));
			dir1.Normalize();
			float3 dif2(oldSmoke-camera->pos);
			dif2.Normalize();
			float3 dir2(dif2.cross(oldSmokeDir));
			dir2.Normalize();


			float a1=(1-float(0)/(Smoke_Time))*255;
			a1*=0.7f+fabs(dif.dot(dir));
			int alpha=min(255,(int)max(0.f,a1));
			col[0]=(unsigned char) (color*alpha);
			col[1]=(unsigned char) (color*alpha);
			col[2]=(unsigned char) (color*alpha);
			col[3]=(unsigned char)alpha;

			float a2=(1-float(age2)/(Smoke_Time))*255;
			a2*=0.7f+fabs(dif2.dot(oldSmokeDir));
			if(age<8)
				a2=0;
			alpha=min(255,(int)max(0.f,a2));
			col2[0]=(unsigned char) (color*alpha);
			col2[1]=(unsigned char) (color*alpha);
			col2[2]=(unsigned char) (color*alpha);
			col2[3]=(unsigned char)alpha;

			float size=1;
			float size2=(1+age2*(1/Smoke_Time)*7);

			float txs=weaponDef->visuals.texture2->xend - (weaponDef->visuals.texture2->xend-weaponDef->visuals.texture2->xstart)*(age2/8.0f);//(1-age2/8.0f);
			va->AddVertexTC(interPos-dir1*size, txs, weaponDef->visuals.texture2->ystart, col);
			va->AddVertexTC(interPos+dir1*size, txs, weaponDef->visuals.texture2->yend, col);
			va->AddVertexTC(oldSmoke+dir2*size2, weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->yend, col2);
			va->AddVertexTC(oldSmoke-dir2*size2, weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->ystart, col2);
		} else {	//draw the trail as particles
			float dist=pos.distance(oldSmoke);
			float3 dirpos1=pos-dir*dist*0.33f;
			float3 dirpos2=oldSmoke+oldSmokeDir*dist*0.33f;

			for(int a=0;a<numParts;++a){
				//float a1=1-float(a)/Smoke_Time;
				col[0]=(unsigned char) (color*255);
				col[1]=(unsigned char) (color*255);
				col[2]=(unsigned char) (color*255);
				col[3]=255;//min(255,max(0,a1*255));
				float size=(1+(a)*(1/Smoke_Time)*7);

				float3 pos1=CalcBeizer(float(a)/(numParts),pos,dirpos1,dirpos2,oldSmoke);
				va->AddVertexTC(pos1+( camera->up+camera->right)*size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
				va->AddVertexTC(pos1+( camera->up-camera->right)*size, ph->smoketex[0].xend, ph->smoketex[0].ystart, col);
				va->AddVertexTC(pos1+(-camera->up-camera->right)*size, ph->smoketex[0].xend, ph->smoketex[0].ystart, col);
				va->AddVertexTC(pos1+(-camera->up+camera->right)*size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
			}

		}
	DrawCallback();
	if(curCallback==0)
		DrawCallback();
}

void CStarburstProjectile::DrawCallback(void)
{
	float3 interPos=pos+speed*gu->timeOffset;

	(*numCallback)++;
	if(*numCallback<2)
		return;
	*numCallback=0;

	inArray=true;
	unsigned char col[4];

	for(int age=0;age<5;++age){
		float3 opos=oldInfos[age]->pos;
		float3 odir=oldInfos[age]->dir;
		float	ospeed=oldInfos[age]->speedf;
		bool createAgeMods=oldInfos[age]->ageMods.empty();
		for(float a=0;a<ospeed+0.6f;a+=0.15f){
			float ageMod;
			if(createAgeMods){
				if(missileAge<20)
					ageMod=1;
				else
					ageMod=0.6f+rand()*0.8f/RAND_MAX;
				oldInfos[age]->ageMods.push_back(ageMod);
			} else {
				ageMod=oldInfos[age]->ageMods[(int)(a/0.15f)];
			}
			float age2=((age+a/(ospeed+0.01f)))*0.2f;
			float3 interPos=opos-odir*(age*0.5f+a);
			float drawsize;
			col[3]=1;
			if(missileAge<20){
				float alpha=max(0.f,((1-age2)*(1-age2)));
				col[0]=(unsigned char) (255*alpha);
				col[1]=(unsigned char) (200*alpha);
				col[2]=(unsigned char) (150*alpha);
			} else {
				float alpha=max(0.f,((1-age2)*max(0.f,(1-age2))));
				col[0]=(unsigned char) (255*alpha);
				col[1]=(unsigned char) (200*alpha);
				col[2]=(unsigned char) (150*alpha);
			}
			drawsize=1+age2*0.8f*ageMod*7;
			va->AddVertexTC(interPos-camera->right*drawsize-camera->up*drawsize,weaponDef->visuals.texture3->xstart,weaponDef->visuals.texture3->ystart,col);
			va->AddVertexTC(interPos+camera->right*drawsize-camera->up*drawsize,weaponDef->visuals.texture3->xend,weaponDef->visuals.texture3->ystart,col);
			va->AddVertexTC(interPos+camera->right*drawsize+camera->up*drawsize,weaponDef->visuals.texture3->xend,weaponDef->visuals.texture3->yend,col);
			va->AddVertexTC(interPos-camera->right*drawsize+camera->up*drawsize,weaponDef->visuals.texture3->xstart,weaponDef->visuals.texture3->yend,col);
		}
	}

	//rita flaren
	col[0]=255;
	col[1]=180;
	col[2]=180;
	col[3]=1;
	float fsize = 25.0f;
	va->AddVertexTC(interPos-camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->ystart,col);
	va->AddVertexTC(interPos+camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->ystart,col);
	va->AddVertexTC(interPos+camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->yend,col);
	va->AddVertexTC(interPos-camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->yend,col);
}

void CStarburstProjectile::DrawUnitPart(void)
{
	float3 interPos=pos+speed*gu->timeOffset;
	glPushMatrix();
	float3 rightdir;
	if(dir.y!=1)
		rightdir=dir.cross(UpVector);
	else
		rightdir=float3(1,0,0);
	rightdir.Normalize();
	float3 updir=rightdir.cross(dir);

	CMatrix44f transMatrix;
	transMatrix[0]=-rightdir.x;
	transMatrix[1]=-rightdir.y;
	transMatrix[2]=-rightdir.z;
	transMatrix[4]=updir.x;
	transMatrix[5]=updir.y;
	transMatrix[6]=updir.z;
	transMatrix[8]=dir.x;
	transMatrix[9]=dir.y;
	transMatrix[10]=dir.z;
	transMatrix[12]=interPos.x;
	transMatrix[13]=interPos.y;
	transMatrix[14]=interPos.z;
	glMultMatrixf(&transMatrix[0]);

	glCallList(modelDispList);
	glPopMatrix();
}

int CStarburstProjectile::ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed)
{
	float3 sdir=pos-shieldPos;
	sdir.Normalize();
	if(ttl > 0){
		float3 dif2=sdir-dir;
		float tracking=max(shieldForce*0.05f,weaponDef->turnrate*2);		//steer away twice as fast as we can steer toward target
		if(dif2.Length()<tracking){
			dir=sdir;
		} else {
			dif2-=dir*(dif2.dot(dir));
			dif2.Normalize();
			dir+=dif2*tracking;
			dir.Normalize();
		}
		return 2;
	}
	return 0;
}
