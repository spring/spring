#include "StdAfx.h"
#include "MissileProjectile.h"
#include "myGL.h"
#include "VertexArray.h"
#include "Camera.h"
#include "Unit.h"
#include "SmokeTrailProjectile.h"
#include "Ground.h"
#include "GameHelper.h"
#include "myMath.h"
#include "3DOParser.h"
#include "Matrix44f.h"
#include "WeaponDefHandler.h"
#include "SyncTracer.h"
#include "GeometricObjects.h"
#include "InfoConsole.h"
//#include "mmgr.h"

static const float Smoke_Time=60;

CMissileProjectile::CMissileProjectile(const float3& pos,const float3& speed,CUnit* owner,const DamageArray& damages,float areaOfEffect,float maxSpeed, int ttl,CUnit* target, WeaponDef *weaponDef,float3 targetPos)
: CWeaponProjectile(pos,speed,owner,target,ZeroVector,weaponDef,0),
	damages(damages),
	ttl(ttl),
	maxSpeed(maxSpeed),
	target(target),
	dir(speed),
	oldSmoke(pos),
	age(0),
	drawTrail(true),
	numParts(0),
	areaOfEffect(areaOfEffect),
	decoyTarget(0),
	targPos(targetPos),
	wobbleTime(1),
	wobbleDir(0,0,0),
	wobbleDif(0,0,0),
	isWobbling(weaponDef->wobble>0),
	extraHeightTime(0)
{
	curSpeed=speed.Length();
	dir.Normalize();
	oldDir=dir;
	if(target)
		AddDeathDependence(target);

	SetRadius(0.0);
	if(!weaponDef->visuals.modelName.empty()){
		S3DOModel* model = unit3doparser->Load3DO(string("objects3d/")+weaponDef->visuals.modelName+".3do",1,0);
		if(model){
			SetRadius(model->radius);
			modelDispList= model->rootobject->displist;
			isUnitPart=true;
		}
	}

	drawRadius=radius+maxSpeed*8;
	ENTER_MIXED;
	float3 camDir=(pos-camera->pos).Normalize();
	if(camera->pos.distance(pos)*0.2+(1-fabs(camDir.dot(dir)))*3000 < 200)
		drawTrail=false;
	ENTER_SYNCED;
	castShadow=true;
#ifdef TRACE_SYNC
	tracefile << "New missile: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif
	if(target)
		target->IncomingMissile(this);

	if(weaponDef->trajectoryHeight>0){
		float dist=pos.distance(targPos);
		extraHeight=dist*weaponDef->trajectoryHeight;
		extraHeightTime=(int)(dist/*+pos.distance(targPos+UpVector*dist))*0.5*//maxSpeed);
		extraHeightDecay=extraHeight/extraHeightTime;
	}
}

CMissileProjectile::~CMissileProjectile(void)
{
}

void CMissileProjectile::DependentDied(CObject* o)
{
	if(o==target)
		target=0;
	if(o==decoyTarget)
		decoyTarget=0;
	CWeaponProjectile::DependentDied(o);
}

void CMissileProjectile::Collision()
{
	float h=ground->GetHeight2(pos.x,pos.z);
	if(h>pos.y)
		pos-=speed*std::min((float)1,float((h-pos.y)/fabs(speed.y)));
	CSmokeTrailProjectile* tp=new CSmokeTrailProjectile(pos,oldSmoke,dir,oldDir,owner,false,true,7,Smoke_Time,0.6f,drawTrail);
	//helper->Explosion(pos,damages,areaOfEffect,owner);
	CWeaponProjectile::Collision();
	oldSmoke=pos;
}

void CMissileProjectile::Collision(CUnit *unit)
{
	CSmokeTrailProjectile* tp=new CSmokeTrailProjectile(pos,oldSmoke,dir,oldDir,owner,false,true,7,Smoke_Time,0.6f,drawTrail);
//	unit->DoDamage(damages,owner);
	//helper->Explosion(pos,damages,areaOfEffect,owner);

	CWeaponProjectile::Collision(unit);
	oldSmoke=pos;
}

void CMissileProjectile::Update(void)
{
	ttl--;
	if(ttl>0){
		if(curSpeed<maxSpeed)
			curSpeed+=weaponDef->weaponacceleration;

		float3 targSpeed(0,0,0);
		if(weaponDef->tracks && (decoyTarget || target)){	
			if(decoyTarget){
				targPos=decoyTarget->pos;
				targSpeed=decoyTarget->speed;
			} else {
				targSpeed=target->speed;
				if((target->midPos-pos).SqLength()<150*150 || !owner)
					targPos=target->midPos;
				else
					targPos=helper->GetUnitErrorPos(target,owner->allyteam);
			}
		}

		if(isWobbling){
			--wobbleTime;
			if(wobbleTime==0){
				float3 newWob=gs->randVector();
				wobbleDif=(newWob-wobbleDir)*(1.0f/16);
				wobbleTime=16;
			}
			wobbleDir+=wobbleDif;
			dir+=wobbleDir*weaponDef->wobble;
			dir.Normalize();
		}

		float3 orgTargPos(targPos);
		if(extraHeightTime){
			extraHeight-=extraHeightDecay;
			--extraHeightTime;
			targPos.y+=extraHeight;
			dir.y-=extraHeightDecay/targPos.distance(pos);
			//geometricObjects->AddLine(pos,targPos,3,1,1);
		}
		float dist=targPos.distance(pos);
		float3 dif(targPos + targSpeed*(dist/maxSpeed)*0.7 - pos);
		dif.Normalize();
		float3 dif2=dif-dir;
		float tracking=weaponDef->turnrate;
		if(dif2.Length()<tracking){
			dir=dif;
		} else {
			dif2-=dir*(dif2.dot(dir));
			dif2.Normalize();
			dir+=dif2*tracking;
			dir.Normalize();
		}

		speed=dir*curSpeed;

		targPos=orgTargPos;
	} else {
		speed*=0.995;
		speed.y+=gs->gravity;
		dir=speed;
		dir.Normalize();
	}
	pos+=speed;

	age++;
	numParts++;
	if(!(age&7)){
		CSmokeTrailProjectile* tp=new CSmokeTrailProjectile(pos,oldSmoke,dir,oldDir,owner,age==8,false,7,Smoke_Time,0.6f,drawTrail);
		oldSmoke=pos;
		oldDir=dir;
		numParts=0;
		useAirLos=tp->useAirLos;
		if(!drawTrail){
			ENTER_MIXED;
			float3 camDir=(pos-camera->pos).Normalize();
			if(camera->pos.distance(pos)*0.2+(1-fabs(camDir.dot(dir)))*3000 > 300)
				drawTrail=true;
			ENTER_SYNCED;
		}
	}

	//CWeaponProjectile::Update();
}

void CMissileProjectile::Draw(void)
{
	float3 interPos=pos+speed*gu->timeOffset;
	inArray=true;
	float age2=(age&7)+gu->timeOffset;

	float color=0.6;
	unsigned char col[4];

	if(drawTrail){		//draw the trail as a single quad
		float3 dif(interPos-camera->pos);
		dif.Normalize();
		float3 dir1(dif.cross(dir));
		dir1.Normalize();
		float3 dif2(oldSmoke-camera->pos);
		dif2.Normalize();
		float3 dir2(dif2.cross(oldDir));
		dir2.Normalize();


		float a1=(1-float(0)/(Smoke_Time))*255;
		a1*=0.7+fabs(dif.dot(dir));
		float alpha=min((float)255,max(float(0),a1));
		col[0]=(unsigned char) (color*alpha);
		col[1]=(unsigned char) (color*alpha);
		col[2]=(unsigned char) (color*alpha);
		col[3]=(unsigned char) alpha;

		unsigned char col2[4];
		float a2=(1-float(age2)/(Smoke_Time))*255;
		if(age<8)
			a2=0;
		a2*=0.7+fabs(dif2.dot(oldDir));
		alpha=min((float)255,max((float)0,a2));
		col2[0]=(unsigned char) (color*alpha);
		col2[1]=(unsigned char) (color*alpha);
		col2[2]=(unsigned char) (color*alpha);
		col2[3]=(unsigned char) alpha;

		float xmod=0;
		float ymod=0.25;
		float size=(1);
		float size2=(1+(age2*(1/Smoke_Time))*7);

		float txs=(1-age2/8.0);
		va->AddVertexTC(interPos-dir1*size, txs/4+1.0/32, 2.0/16, col);
		va->AddVertexTC(interPos+dir1*size, txs/4+1.0/32, 3.0/16, col);
		va->AddVertexTC(oldSmoke+dir2*size2, 0.25+1.0/32, 3.0/16, col2);
		va->AddVertexTC(oldSmoke-dir2*size2, 0.25+1.0/32, 2.0/16, col2);
	} else {	//draw the trail as particles
		float dist=pos.distance(oldSmoke);
		float3 dirpos1=pos-dir*dist*0.33;
		float3 dirpos2=oldSmoke+oldDir*dist*0.33;

		for(int a=0;a<numParts;++a){
			float a1=1-float(a)/Smoke_Time;
			float alpha=255;
			col[0]=(unsigned char) (color*alpha);
			col[1]=(unsigned char) (color*alpha);
			col[2]=(unsigned char) (color*alpha);
			col[3]=(unsigned char)alpha;//min(255,max(0,a1*255));
			float size=(1+(a*(1/Smoke_Time))*7);

			float3 pos1=CalcBeizer(float(a)/(numParts),pos,dirpos1,dirpos2,oldSmoke);
			va->AddVertexTC(pos1+( camera->up+camera->right)*size, 4.0/16, 0.0/16, col);
			va->AddVertexTC(pos1+( camera->up-camera->right)*size, 5.0/16, 0.0/16, col);
			va->AddVertexTC(pos1+(-camera->up-camera->right)*size, 5.0/16, 1.0/16, col);
			va->AddVertexTC(pos1+(-camera->up+camera->right)*size, 4.0/16, 1.0/16, col);
		}

	}
	//rita flaren
	col[0]=255;
	col[1]=210;
	col[2]=180;
	col[3]=1;
	float fsize = radius*0.4;
	va->AddVertexTC(interPos-camera->right*fsize-camera->up*fsize,0.51,0.13,col);
	va->AddVertexTC(interPos+camera->right*fsize-camera->up*fsize,0.99,0.13,col);
	va->AddVertexTC(interPos+camera->right*fsize+camera->up*fsize,0.99,0.36,col);
	va->AddVertexTC(interPos-camera->right*fsize+camera->up*fsize,0.51,0.36,col);

/*	col[0]=200;
	col[1]=200;
	col[2]=200;
	col[3]=255;
	float3 r=dir.cross(UpVector);
	r.Normalize();
	float3 u=dir.cross(r);
	va->AddVertexTC(interPos+r*1.0f,1.0/16,1.0/16,col);
	va->AddVertexTC(interPos-r*1.0f,1.0/16,1.0/16,col);
	va->AddVertexTC(interPos+dir*9,1.0/16,1.0/16,col);
	va->AddVertexTC(interPos+dir*9,1.0/16,1.0/16,col);

	va->AddVertexTC(interPos+u*1.0f,1.0/16,1.0/16,col);
	va->AddVertexTC(interPos-u*1.0f,1.0/16,1.0/16,col);
	va->AddVertexTC(interPos+dir*9,1.0/16,1.0/16,col);
	va->AddVertexTC(interPos+dir*9,1.0/16,1.0/16,col);*/
}

void CMissileProjectile::DrawUnitPart(void)
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
	transMatrix[12]=interPos.x+dir.x*radius*0.9;
	transMatrix[13]=interPos.y+dir.y*radius*0.9;
	transMatrix[14]=interPos.z+dir.z*radius*0.9;
	glMultMatrixf(&transMatrix[0]);

	glCallList(modelDispList);
	glPopMatrix();
}
