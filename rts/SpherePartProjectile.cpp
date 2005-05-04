#include "stdafx.h"
#include ".\spherepartprojectile.h"
#include "mygl.h"
#include "vertexarray.h"
//#include "mmgr.h"

CSpherePartProjectile::CSpherePartProjectile(const float3& centerPos,int xpart,int ypart,float expansionSpeed,float alpha,int ttl,CUnit* owner)
: CProjectile(centerPos,ZeroVector,owner),
	centerPos(centerPos),
	expansionSpeed(expansionSpeed),
	sphereSize(expansionSpeed),
	age(0),
	ttl(ttl),
	baseAlpha(alpha),
	xbase(xpart),
	ybase(ypart)
{
	checkCol=false;

	for(int y=0;y<5;++y){
		float yp=(y+ypart)/16.0*PI-PI/2;
		for(int x=0;x<5;++x){
			float xp=(x+xpart)/32.0*2*PI;
			vectors[y*5+x]=float3(sin(xp)*cos(yp),sin(yp),cos(xp)*cos(yp));
		}
	}
	pos=centerPos+vectors[12]*sphereSize;

	drawRadius=60;
	alwaysVisible=true;
}

CSpherePartProjectile::~CSpherePartProjectile(void)
{
}

void CSpherePartProjectile::Update(void)
{
	age++;
	if(age>=ttl)
		deleteMe=true;
	sphereSize+=expansionSpeed;
	pos=centerPos+vectors[12]*sphereSize;
}

void CSpherePartProjectile::Draw(void)
{
	unsigned char col[4];

	float interSize=sphereSize+expansionSpeed*gu->timeOffset;
	for(int y=0;y<4;++y){
		for(int x=0;x<4;++x){
			float alpha=baseAlpha*(1.0-min(1.0,float(age+gu->timeOffset)/ttl))*(1-fabs(y+ybase-8.0f)/8.0*1.0);

			col[0]=200*alpha;
			col[1]=200*alpha;
			col[2]=150*alpha;
			col[3]=40*alpha;
			va->AddVertexTC(centerPos+vectors[y*5+x]*interSize,1.0/16,1.0/16,col);
			va->AddVertexTC(centerPos+vectors[y*5+x+1]*interSize,1.0/16,1.0/16,col);
			alpha=baseAlpha*(1.0-min(1.0,float(age+gu->timeOffset)/ttl))*(1-fabs(y+1+ybase-8.0f)/8.0*1.0);

			col[0]=200*alpha;
			col[1]=200*alpha;
			col[2]=150*alpha;
			col[3]=40*alpha;
			va->AddVertexTC(centerPos+vectors[(y+1)*5+x+1]*interSize,1.0/16,1.0/16,col);
			va->AddVertexTC(centerPos+vectors[(y+1)*5+x]*interSize,1.0/16,1.0/16,col);
		}
	}
}


void CSpherePartProjectile::CreateSphere(float3 pos, float alpha, int ttl, float expansionSpeed , CUnit* owner)
{
	for(int y=0;y<16;y+=4){
		for(int x=0;x<32;x+=4){
			new CSpherePartProjectile(pos,x,y,expansionSpeed,alpha,ttl,owner);
		}
	}
}
