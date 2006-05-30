#include "StdAfx.h"
#include "RepulseGfx.h"
#include "Sim/Units/Unit.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "mmgr.h"

CRepulseGfx::CRepulseGfx(CUnit* owner,CProjectile* repulsed,float maxDist,float3 color)
: CProjectile(repulsed->pos,repulsed->speed,owner),
	repulsed(repulsed),
	age(0),
	sqMaxDist(maxDist*maxDist+100),
	color(color)
{
	AddDeathDependence(owner);
	AddDeathDependence(repulsed);

	checkCol=false;
	useAirLos=true;
	SetRadius(maxDist);

	for(int y=0;y<5;++y){
		float yp=(y/4.0-0.5);
		for(int x=0;x<5;++x){
			float xp=(x/4.0-0.5);
			float d=sqrt(xp*xp+yp*yp);
			difs[y*5+x]=(1-cos(d*2))*20;
		}
	}
}

CRepulseGfx::~CRepulseGfx(void)
{
}

void CRepulseGfx::DependentDied(CObject* o)
{
	if(o==owner){
		owner=0;
		deleteMe=true;
	}
	if(o==repulsed){
		repulsed=0;
		deleteMe=true;
	}
	CProjectile::DependentDied(o);
}

void CRepulseGfx::Draw(void)
{
	if(!owner || !repulsed)
		return;

	float3 dir=repulsed->pos-owner->pos;
	dir.Normalize();

	pos=repulsed->pos-dir*10+repulsed->speed*gu->timeOffset;

	float3 dir1=dir.cross(UpVector);
	dir1.Normalize();
	float3 dir2=dir1.cross(dir);

	inArray=true;
	unsigned char col[4];
	float alpha=min(255,age*10);
	col[0]=(unsigned char)(color.x*alpha);
	col[1]=(unsigned char)(color.y*alpha);
	col[2]=(unsigned char)(color.z*alpha);
	col[3]=(unsigned char)(alpha*0.2);
	float drawsize=10;
	float3 interPos=pos+speed*gu->timeOffset;

	for(int y=0;y<4;++y){
		float dy=y-2;
		float ry=y*0.25;
		for(int x=0;x<4;++x){
			float dx=x-2;
			float rx=x*0.25;
			va->AddVertexTC(pos+dir1*drawsize*(dx+0)+dir2*drawsize*(dy+0)+dir*difs[y*5+x]			 ,0.0+ry*0.25			,0.25+(rx)*0.25,col);
			va->AddVertexTC(pos+dir1*drawsize*(dx+0)+dir2*drawsize*(dy+1)+dir*difs[(y+1)*5+x]  ,0.0+(ry+0.25)*0.25,0.25+(rx)*0.25,col);
			va->AddVertexTC(pos+dir1*drawsize*(dx+1)+dir2*drawsize*(dy+1)+dir*difs[(y+1)*5+x+1],0.0+(ry+0.25)*0.25,0.25+(rx+0.25)*0.25,col);
			va->AddVertexTC(pos+dir1*drawsize*(dx+1)+dir2*drawsize*(dy+0)+dir*difs[y*5+x+1]		 ,0.0+ry*0.25			,0.25+(rx+0.25)*0.25,col);
		}
	}
	drawsize=7;
	alpha=min(10,age/2);
	col[0]=(unsigned char)(color.x*alpha);
	col[1]=(unsigned char)(color.y*alpha);
	col[2]=(unsigned char)(color.z*alpha);
	col[3]=(unsigned char)(alpha*0.4);

	unsigned char col2[4];
	col2[0]=0;
	col2[1]=0;
	col2[2]=0;
	col2[3]=0;

	va->AddVertexTC(owner->pos+(-dir1+dir2)*drawsize*0.2,0.0625,0.0625,col2);
	va->AddVertexTC(owner->pos+(dir1+dir2)*drawsize*0.2,0.0625,0.0625,col2);
	va->AddVertexTC(pos+dir1*drawsize+dir2*drawsize+dir*difs[6],0.0625,0.0625,col);
	va->AddVertexTC(pos-dir1*drawsize+dir2*drawsize+dir*difs[6],0.0625,0.0625,col);

	va->AddVertexTC(owner->pos+(-dir1-dir2)*drawsize*0.2,0.0625,0.0625,col2);
	va->AddVertexTC(owner->pos+(dir1-dir2)*drawsize*0.2,0.0625,0.0625,col2);
	va->AddVertexTC(pos+dir1*drawsize-dir2*drawsize+dir*difs[6],0.0625,0.0625,col);
	va->AddVertexTC(pos-dir1*drawsize-dir2*drawsize+dir*difs[6],0.0625,0.0625,col);

	va->AddVertexTC(owner->pos+(dir1-dir2)*drawsize*0.2,0.0625,0.0625,col2);
	va->AddVertexTC(owner->pos+(dir1+dir2)*drawsize*0.2,0.0625,0.0625,col2);
	va->AddVertexTC(pos+dir1*drawsize+dir2*drawsize+dir*difs[6],0.0625,0.0625,col);
	va->AddVertexTC(pos+dir1*drawsize-dir2*drawsize+dir*difs[6],0.0625,0.0625,col);

	va->AddVertexTC(owner->pos+(-dir1-dir2)*drawsize*0.2,0.0625,0.0625,col2);
	va->AddVertexTC(owner->pos+(-dir1+dir2)*drawsize*0.2,0.0625,0.0625,col2);
	va->AddVertexTC(pos-dir1*drawsize+dir2*drawsize+dir*difs[6],0.0625,0.0625,col);
	va->AddVertexTC(pos-dir1*drawsize-dir2*drawsize+dir*difs[6],0.0625,0.0625,col);
}

void CRepulseGfx::Update(void)
{
	age++;

	if(repulsed && owner && (repulsed->pos-owner->pos).SqLength()>sqMaxDist)
		deleteMe=true;
}
