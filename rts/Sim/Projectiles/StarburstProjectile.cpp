#include "StdAfx.h"
#include "StarburstProjectile.h"
#include "myGL.h"
#include "VertexArray.h"
#include "Camera.h"
#include "Unit.h"
#include "SmokeTrailProjectile.h"
#include "Ground.h"
#include "GameHelper.h"
#include "myMath.h"
#include "WeaponDefHandler.h"
#include "3DOParser.h"
#include "Matrix44f.h"
#include "SyncTracer.h"
//#include "mmgr.h"

static const float Smoke_Time=70;

CStarburstProjectile::CStarburstProjectile(const float3& pos,const float3& speed,CUnit* owner,float3 targetPos,const DamageArray& damages,float areaOfEffect,float maxSpeed,float tracking, int uptime,CUnit* target, WeaponDef *weaponDef, CWeaponProjectile* interceptTarget)
: CWeaponProjectile(pos,speed,owner,target,targetPos,weaponDef,interceptTarget),
	damages(damages),
	ttl(200),
	maxSpeed(maxSpeed),
	tracking(tracking),
	target(target),
	dir(speed),
	oldSmoke(pos),
	age(0),
	drawTrail(true),
	numParts(0),
	targetPos(targetPos),
	doturn(true),
	curCallback(0),
	numCallback(0),
	missileAge(0),
	areaOfEffect(areaOfEffect)
{
	this->uptime=uptime;
	ttl=(int)min(3000.f,uptime+weaponDef->range/maxSpeed+100);

	maxGoodDif=cos(tracking*0.6);
	curSpeed=speed.Length();
	dir.Normalize();
	oldSmokeDir=dir;
	if(target){
		AddDeathDependence(target);
	}
	drawRadius=maxSpeed*8;
	ENTER_MIXED;
	numCallback=new int;
	*numCallback=0;
	float3 camDir=(pos-camera->pos).Normalize();
	if(camera->pos.distance(pos)*0.2+(1-fabs(camDir.dot(dir)))*3000 < 200)
		drawTrail=false;
	ENTER_SYNCED;

	for(int a=0;a<5;++a){
		oldInfos[a]=new OldInfo;
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

void CStarburstProjectile::DependentDied(CObject* o)
{
	if(o==target)
		target=0;
	CWeaponProjectile::DependentDied(o);
}

void CStarburstProjectile::Collision()
{
	float h=ground->GetHeight2(pos.x,pos.z);
	if(h>pos.y)
		pos+=speed*(h-pos.y)/speed.y;
	CSmokeTrailProjectile* tp=new CSmokeTrailProjectile(pos,oldSmoke,dir,oldSmokeDir,owner,false,true,7,Smoke_Time,0.7f,drawTrail);
	oldSmokeDir=dir;
//	helper->Explosion(pos,damages,areaOfEffect,owner);
	CWeaponProjectile::Collision();
	oldSmoke=pos;
}

void CStarburstProjectile::Collision(CUnit *unit)
{
	CSmokeTrailProjectile* tp=new CSmokeTrailProjectile(pos,oldSmoke,dir,oldSmokeDir,owner,false,true,7,Smoke_Time,0.7f,drawTrail);
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
			curSpeed+=0.1;
		dir=UpVector;
		speed=dir*curSpeed;
	} else if(doturn && ttl>0){
		float3 dif(targetPos-pos);
		dif.Normalize();
		if(dif.dot(dir)>0.99){
			dir=dif;
			doturn=false;
		} else {
			dif=dif-dir;
			dif-=dir*(dif.dot(dir));
			dif.Normalize();
			dir+=dif*0.06;
			dir.Normalize();
		}
		speed=dir*curSpeed;
	} else if(ttl>0){
		if(curSpeed<maxSpeed)
			curSpeed+=0.1;
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
	if(!(age&7)){
		if(curCallback)
			curCallback->drawCallbacker=0;
		curCallback=new CSmokeTrailProjectile(pos,oldSmoke,dir,oldSmokeDir,owner,age==8,false,7,Smoke_Time,0.7f,drawTrail,this);
		oldSmoke=pos;
		oldSmokeDir=dir;
		numParts=0;
		useAirLos=curCallback->useAirLos;
		if(!drawTrail){
			ENTER_MIXED;
			float3 camDir=(pos-camera->pos).Normalize();
			if(camera->pos.distance(pos)*0.2+(1-fabs(camDir.dot(dir)))*3000 > 300)
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

	float color=0.7;
	unsigned char col[4];
	unsigned char col2[4];

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
		a1*=0.7+fabs(dif.dot(dir));
		int alpha=min(255,(int)max(0.f,a1));
		col[0]=(unsigned char) (color*alpha);
		col[1]=(unsigned char) (color*alpha);
		col[2]=(unsigned char) (color*alpha);
		col[3]=(unsigned char)alpha;

		float a2=(1-float(age2)/(Smoke_Time))*255;
		a2*=0.7+fabs(dif2.dot(oldSmokeDir));
		if(age<8)
			a2=0;
		alpha=min(255,(int)max(0.f,a2));
		col2[0]=(unsigned char) (color*alpha);
		col2[1]=(unsigned char) (color*alpha);
		col2[2]=(unsigned char) (color*alpha);
		col2[3]=(unsigned char)alpha;

		float xmod=0;
		float ymod=0.25;
		float size=1;
		float size2=(1+age2*(1/Smoke_Time)*7);

		float txs=(1-age2/8.0);
		va->AddVertexTC(interPos-dir1*size, txs/4+1.0/32, 2.0/16, col);
		va->AddVertexTC(interPos+dir1*size, txs/4+1.0/32, 3.0/16, col);
		va->AddVertexTC(oldSmoke+dir2*size2, 0.25+1.0/32, 3.0/16, col2);
		va->AddVertexTC(oldSmoke-dir2*size2, 0.25+1.0/32, 2.0/16, col2);
	} else {	//draw the trail as particles
		float dist=pos.distance(oldSmoke);
		float3 dirpos1=pos-dir*dist*0.33;
		float3 dirpos2=oldSmoke+oldSmokeDir*dist*0.33;

		for(int a=0;a<numParts;++a){
			float a1=1-float(a)/Smoke_Time;
			col[0]=(unsigned char) (color*255);
			col[1]=(unsigned char) (color*255);
			col[2]=(unsigned char) (color*255);
			col[3]=255;//min(255,max(0,a1*255));
			float size=(1+(a)*(1/Smoke_Time)*7);

			float3 pos1=CalcBeizer(float(a)/(numParts),pos,dirpos1,dirpos2,oldSmoke);
			va->AddVertexTC(pos1+( camera->up+camera->right)*size, 4.0/16, 0.0/16, col);
			va->AddVertexTC(pos1+( camera->up-camera->right)*size, 5.0/16, 0.0/16, col);
			va->AddVertexTC(pos1+(-camera->up-camera->right)*size, 5.0/16, 1.0/16, col);
			va->AddVertexTC(pos1+(-camera->up+camera->right)*size, 4.0/16, 1.0/16, col);
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
		for(float a=0;a<ospeed+0.6;a+=0.15){
			float ageMod;
			if(createAgeMods){
				if(missileAge<20)
					ageMod=1;
				else
					ageMod=0.6+rand()*0.8/RAND_MAX;
				oldInfos[age]->ageMods.push_back(ageMod);
			} else {
				ageMod=oldInfos[age]->ageMods[(int)(a/0.15)];
			}
			float age2=((age+a/(ospeed+0.01)))*0.2;
			float3 interPos=opos-odir*(age*0.5+a);
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
			drawsize=1+age2*0.8*ageMod*7;
			va->AddVertexTC(interPos-camera->right*drawsize-camera->up*drawsize,0.25,0.25,col);
			va->AddVertexTC(interPos+camera->right*drawsize-camera->up*drawsize,0.5,0.25,col);
			va->AddVertexTC(interPos+camera->right*drawsize+camera->up*drawsize,0.5,0.5,col);
			va->AddVertexTC(interPos-camera->right*drawsize+camera->up*drawsize,0.25,0.5,col);
		}
	}

	//rita flaren
	col[0]=255;
	col[1]=180;
	col[2]=180;
	col[3]=1;
	float fsize = 25.0f;
	va->AddVertexTC(interPos-camera->right*fsize-camera->up*fsize,0.51,0.13,col);
	va->AddVertexTC(interPos+camera->right*fsize-camera->up*fsize,0.99,0.13,col);
	va->AddVertexTC(interPos+camera->right*fsize+camera->up*fsize,0.99,0.36,col);
	va->AddVertexTC(interPos-camera->right*fsize+camera->up*fsize,0.51,0.36,col);
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
