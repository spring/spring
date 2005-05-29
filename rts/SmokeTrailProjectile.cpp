// SmokeTrailProjectile.cpp: implementation of the CSmokeTrailProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SmokeTrailProjectile.h"
#include "ProjectileHandler.h"
#include "Camera.h"
#include "myGL.h"
#include "VertexArray.h"
#include "Ground.h"
#include "myMath.h"
#include "Wind.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSmokeTrailProjectile::CSmokeTrailProjectile(const float3& pos1,const float3& pos2,const float3& dir1,const float3& dir2, CUnit* owner,bool firstSegment,bool lastSegment,float size,float time,float color,bool drawTrail,CProjectile* drawCallback)
: CProjectile((pos1+pos2)*0.5,ZeroVector,owner),
	pos1(pos1),
	pos2(pos2),
	creationTime(gs->frameNum),
	lifeTime(time),
	orgSize(size),
	color(color),
	dir1(dir1),
	dir2(dir2),
	drawTrail(drawTrail),
	drawSegmented(false),
	drawCallbacker(drawCallback),
	firstSegment(firstSegment),
	lastSegment(lastSegment)
{
	checkCol=false;
	castShadow=true;

	if(!drawTrail){
		float dist=pos1.distance(pos2);
		dirpos1=pos1-dir1*dist*0.33;
		dirpos2=pos2+dir2*dist*0.33;
	} else if(dir1.dot(dir2)<0.98){
		float dist=pos1.distance(pos2);
		dirpos1=pos1-dir1*dist*0.33;
		dirpos2=pos2+dir2*dist*0.33;
		float3 mp=(pos1+pos2)/2;
		midpos=CalcBeizer(0.5,pos1,dirpos1,dirpos2,pos2);
		middir=(dir1+dir2).Normalize();
		drawSegmented=true;
	}
	SetRadius(pos1.distance(pos2));

	if(pos.y-ground->GetApproximateHeight(pos.x,pos.z)>10)
		useAirLos=true;
}

CSmokeTrailProjectile::~CSmokeTrailProjectile()
{

}

void CSmokeTrailProjectile::Draw()
{
	inArray=true;
	float age=gs->frameNum+gu->timeOffset-creationTime;

	if(drawTrail){
		float3 dif(pos1-camera->pos2);
		dif.Normalize();
		float3 odir1(dif.cross(dir1));
		odir1.Normalize();
		float3 dif2(pos2-camera->pos2);
		dif2.Normalize();
		float3 odir2(dif2.cross(dir2));
		odir2.Normalize();

		unsigned char col[4];
		float a1=(1-float(age)/(lifeTime))*255;
		if(lastSegment)
			a1=0;
		a1*=0.7+fabs(dif.dot(dir1));
		float alpha=min(255.f,max(0.f,a1));
		col[0]=(unsigned char) (color*alpha);
		col[1]=(unsigned char) (color*alpha);
		col[2]=(unsigned char) (color*alpha);
		col[3]=(unsigned char)alpha;

		unsigned char col2[4];
		float a2=(1-float(age+8)/(lifeTime))*255;
		if(firstSegment)
			a2=0;
		a2*=0.7+fabs(dif2.dot(dir2));
		alpha=min(255.f,max(0.f,a2));
		col2[0]=(unsigned char) (color*alpha);
		col2[1]=(unsigned char) (color*alpha);
		col2[2]=(unsigned char) (color*alpha);
		col2[3]=(unsigned char)alpha;

		float xmod=0;
		float ymod=0.25;
		float size=1+(age*(1.0/lifeTime))*orgSize;
		float size2=1+((age+8)*(1.0/lifeTime))*orgSize;

		if(drawSegmented){
			float3 dif3(midpos-camera->pos2);
			dif3.Normalize();
			float3 odir3(dif3.cross(middir));
			odir3.Normalize();
			float size3=0.2+((age+4)*(1.0/lifeTime))*orgSize;

			unsigned char col3[4];
			float a2=(1-float(age+4)/(lifeTime))*255;
			a2*=0.7+fabs(dif3.dot(middir));
			alpha=min(255.f,max(0.f,a2));
			col3[0]=(unsigned char) (color*alpha);
			col3[1]=(unsigned char) (color*alpha);
			col3[2]=(unsigned char) (color*alpha);
			col3[3]=(unsigned char)alpha;

			va->AddVertexTC(pos1-odir1*size,0+1.0/32,1.0/8,col);
			va->AddVertexTC(pos1+odir1*size,0+1.0/32,3.0/16,col);
			va->AddVertexTC(midpos+odir3*size3,0.125+1.0/32,3.0/16,col3);
			va->AddVertexTC(midpos-odir3*size3,0.125+1.0/32,1.0/8,col3);

			va->AddVertexTC(midpos-odir3*size3,0.125+1.0/32,1.0/8,col3);
			va->AddVertexTC(midpos+odir3*size3,0.125+1.0/32,3.0/16,col3);
			va->AddVertexTC(pos2+odir2*size2,0.25+1.0/32,3.0/16,col2);
			va->AddVertexTC(pos2-odir2*size2,0.25+1.0/32,1.0/8,col2);
		} else {
			va->AddVertexTC(pos1-odir1*size,0+1.0/32,1.0/8,col);
			va->AddVertexTC(pos1+odir1*size,0+1.0/32,3.0/16,col);
			va->AddVertexTC(pos2+odir2*size2,0.25+1.0/32,3.0/16,col2);
			va->AddVertexTC(pos2-odir2*size2,0.25+1.0/32,1.0/8,col2);
		}
	} else {	//draw as particles
		unsigned char col[4];
		for(int a=0;a<8;++a){
			float a1=1-float(age+a)/lifeTime;
			float alpha=min(255.f,max(0.f,a1*255));
			col[0]=(unsigned char) (color*alpha);
			col[1]=(unsigned char) (color*alpha);
			col[2]=(unsigned char) (color*alpha);
			col[3]=(unsigned char)alpha;
			float size=((0.2+(age+a)*(1.0/lifeTime))*orgSize)*1.2;

			float3 pos=CalcBeizer(a/8.0,pos1,dirpos1,dirpos2,pos2);
			va->AddVertexTC(pos+( camera->up+camera->right)*size, 4.0/16, 0.0/16, col);
			va->AddVertexTC(pos+( camera->up-camera->right)*size, 5.0/16, 0.0/16, col);
			va->AddVertexTC(pos+(-camera->up-camera->right)*size, 5.0/16, 1.0/16, col);
			va->AddVertexTC(pos+(-camera->up+camera->right)*size, 4.0/16, 1.0/16, col);
		}
	}
	if(drawCallbacker)
		drawCallbacker->DrawCallback();
}

void CSmokeTrailProjectile::Update()
{
	if(gs->frameNum>=creationTime+lifeTime)
		deleteMe=true;
}
