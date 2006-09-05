#include "StdAfx.h"
#include "FireBallProjectile.h"
#include "Rendering/GL/VertexArray.h"
#include "Game/Camera.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "mmgr.h"
#include "ProjectileHandler.h"

CFireBallProjectile::CFireBallProjectile(const float3& pos,const float3& speed, CUnit* owner, CUnit *target, const float3 &targetPos, WeaponDef *weaponDef)
	: CWeaponProjectile(pos,speed, owner, target, targetPos, weaponDef,damages,0)
{
	SetRadius(weaponDef->size);
	drawRadius=weaponDef->size;
}

CFireBallProjectile::~CFireBallProjectile(void)
{
}

void CFireBallProjectile::Draw()
{
	inArray=true;
	unsigned char col[4] = {255,150, 100, 1};

	float3 interPos = pos;
	if(checkCol)
		interPos+=speed*gu->timeOffset;
	float size = radius*1.3;

	int numSparks=sparks.size();
	for(int i=0; i<numSparks; i++)
	{
		col[0]=(numSparks-i)*12;
		col[1]=(numSparks-i)*6;
		col[2]=(numSparks-i)*4;
		va->AddVertexTC(sparks[i].pos-camera->right*sparks[i].size-camera->up*sparks[i].size,ph->explotex.xstart,ph->explotex.ystart,col);
		va->AddVertexTC(sparks[i].pos+camera->right*sparks[i].size-camera->up*sparks[i].size,ph->explotex.xend ,ph->explotex.ystart,col);
		va->AddVertexTC(sparks[i].pos+camera->right*sparks[i].size+camera->up*sparks[i].size,ph->explotex.xend ,ph->explotex.yend ,col);
		va->AddVertexTC(sparks[i].pos-camera->right*sparks[i].size+camera->up*sparks[i].size,ph->explotex.xstart,ph->explotex.yend ,col);
	}

	int numFire=std::min(10,numSparks);
	int maxCol=numFire;
	if(checkCol)
		maxCol=10;
	for(int i=0; i<numFire; i++)
	{
		col[0]=(maxCol-i)*25;
		col[1]=(maxCol-i)*15;
		col[2]=(maxCol-i)*10;
		va->AddVertexTC(interPos-camera->right*size-camera->up*size,ph->flaretex.xstart ,ph->flaretex.ystart ,col);
		va->AddVertexTC(interPos+camera->right*size-camera->up*size,ph->flaretex.xend ,ph->flaretex.ystart ,col);
		va->AddVertexTC(interPos+camera->right*size+camera->up*size,ph->flaretex.xend ,ph->flaretex.yend ,col);
		va->AddVertexTC(interPos-camera->right*size+camera->up*size,ph->flaretex.xstart ,ph->flaretex.yend ,col);
		interPos = interPos-speed*0.5;
	}
}

void CFireBallProjectile::Update()
{
	if(checkCol)
	{
		pos+=speed;

		if(weaponDef->gravityAffected)
			speed.y+=gs->gravity;

		//göra om till ttl sedan kanske
		if(weaponDef->noExplode)
		{
			if(TraveledRange())
				checkCol=false;
		}

		EmitSpark();
	}
	else
	{
		if(sparks.size()==0)
			deleteMe = true;
	}

	for(unsigned int i=0; i<sparks.size(); i++)
	{
		sparks[i].ttl--;
		if(sparks[i].ttl==0){
			sparks.pop_back();
			break;
		}
		if(checkCol)
			sparks[i].pos += sparks[i].speed;
		sparks[i].speed *= 0.95;
	}
}

void CFireBallProjectile::EmitSpark()
{
	Spark spark;
	spark.speed = speed*0.95 + float3(rand()/(float)RAND_MAX*1.0f-0.5f, rand()/(float)RAND_MAX*1.0f-0.5f, rand()/(float)RAND_MAX*1.0f-0.5f);
	spark.pos = pos-speed*(rand()/(float)RAND_MAX*1.0f+3)+spark.speed*3;
	spark.size = 5.0f;
	spark.ttl = 15;

	sparks.push_front(spark);
}

void CFireBallProjectile::Collision()
{
	CWeaponProjectile::Collision();
	deleteMe = false;
}

