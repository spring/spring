// SmokeTrailProjectile.cpp: implementation of the CSmokeTrailProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "myMath.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "SmokeTrailProjectile.h"
#include "mmgr.h"

CR_BIND_DERIVED(CSmokeTrailProjectile, CProjectile, (float3(0,0,0),float3(0,0,0),float3(0,0,0),float3(0,0,0),NULL,0,0,0,0,0,0,NULL));

CR_REG_METADATA(CSmokeTrailProjectile,(
	CR_MEMBER(pos1),
	CR_MEMBER(pos2),
	CR_MEMBER(orgSize),
	CR_MEMBER(creationTime),
	CR_MEMBER(lifeTime),
	CR_MEMBER(color),
	CR_MEMBER(dir1),
	CR_MEMBER(dir2),
	CR_MEMBER(drawTrail),
	CR_MEMBER(dirpos1),
	CR_MEMBER(dirpos2),
	CR_MEMBER(midpos),
	CR_MEMBER(middir),
	CR_MEMBER(drawSegmented),
	CR_MEMBER(firstSegment),
	CR_MEMBER(lastSegment),
	CR_MEMBER(drawCallbacker)
	));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSmokeTrailProjectile::CSmokeTrailProjectile(const float3& pos1,const float3& pos2,const float3& dir1,const float3& dir2, CUnit* owner,bool firstSegment,bool lastSegment,float size,float time,float color,bool drawTrail,CProjectile* drawCallback)
: CProjectile((pos1+pos2)*0.5f,ZeroVector,owner, false),
	pos1(pos1),
	pos2(pos2),
	creationTime(gs->frameNum),
	lifeTime((int)time),
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
		dirpos1=pos1-dir1*dist*0.33f;
		dirpos2=pos2+dir2*dist*0.33f;
	} else if(dir1.dot(dir2)<0.98f){
		float dist=pos1.distance(pos2);
		dirpos1=pos1-dir1*dist*0.33f;
		dirpos2=pos2+dir2*dist*0.33f;
		float3 mp=(pos1+pos2)/2;
		midpos=CalcBeizer(0.5f,pos1,dirpos1,dirpos2,pos2);
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
		a1*=0.7f+fabs(dif.dot(dir1));
		float alpha=min(255.f,max(0.f,a1));
		col[0]=(unsigned char) (color*alpha);
		col[1]=(unsigned char) (color*alpha);
		col[2]=(unsigned char) (color*alpha);
		col[3]=(unsigned char)alpha;

		unsigned char col2[4];
		float a2=(1-float(age+8)/(lifeTime))*255;
		if(firstSegment)
			a2=0;
		a2*=0.7f+fabs(dif2.dot(dir2));
		alpha=min(255.f,max(0.f,a2));
		col2[0]=(unsigned char) (color*alpha);
		col2[1]=(unsigned char) (color*alpha);
		col2[2]=(unsigned char) (color*alpha);
		col2[3]=(unsigned char)alpha;

		float size=1+(age*(1.0f/lifeTime))*orgSize;
		float size2=1+((age+8)*(1.0f/lifeTime))*orgSize;

		if(drawSegmented){
			float3 dif3(midpos-camera->pos2);
			dif3.Normalize();
			float3 odir3(dif3.cross(middir));
			odir3.Normalize();
			float size3=0.2f+((age+4)*(1.0f/lifeTime))*orgSize;

			unsigned char col3[4];
			float a2=(1-float(age+4)/(lifeTime))*255;
			a2*=0.7f+fabs(dif3.dot(middir));
			alpha=min(255.f,max(0.f,a2));
			col3[0]=(unsigned char) (color*alpha);
			col3[1]=(unsigned char) (color*alpha);
			col3[2]=(unsigned char) (color*alpha);
			col3[3]=(unsigned char)alpha;

			float midtexx = ph->smoketrailtex.xstart + (ph->smoketrailtex.xend - ph->smoketrailtex.xstart)*0.5f;

			va->AddVertexTC(pos1-odir1*size,ph->smoketrailtex.xstart,ph->smoketrailtex.ystart,col);
			va->AddVertexTC(pos1+odir1*size,ph->smoketrailtex.xstart,ph->smoketrailtex.yend,col);
			va->AddVertexTC(midpos+odir3*size3,midtexx,ph->smoketrailtex.yend,col3);
			va->AddVertexTC(midpos-odir3*size3,midtexx,ph->smoketrailtex.ystart,col3);

			va->AddVertexTC(midpos-odir3*size3,midtexx,ph->smoketrailtex.ystart,col3);
			va->AddVertexTC(midpos+odir3*size3,midtexx,ph->smoketrailtex.yend,col3);
			va->AddVertexTC(pos2+odir2*size2,ph->smoketrailtex.xend,ph->smoketrailtex.yend,col2);
			va->AddVertexTC(pos2-odir2*size2,ph->smoketrailtex.xend,ph->smoketrailtex.ystart,col2);
		} else {
			va->AddVertexTC(pos1-odir1*size,ph->smoketrailtex.xstart,ph->smoketrailtex.ystart,col);
			va->AddVertexTC(pos1+odir1*size,ph->smoketrailtex.xstart,ph->smoketrailtex.yend,col);
			va->AddVertexTC(pos2+odir2*size2,ph->smoketrailtex.xend,ph->smoketrailtex.yend,col2);
			va->AddVertexTC(pos2-odir2*size2,ph->smoketrailtex.xend,ph->smoketrailtex.ystart,col2);
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
			float size=((0.2f+(age+a)*(1.0f/lifeTime))*orgSize)*1.2f;

			float3 pos=CalcBeizer(a/8.0f,pos1,dirpos1,dirpos2,pos2);
			va->AddVertexTC(pos1+( camera->up+camera->right)*size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
			va->AddVertexTC(pos1+( camera->up-camera->right)*size, ph->smoketex[0].xend, ph->smoketex[0].ystart, col);
			va->AddVertexTC(pos1+(-camera->up-camera->right)*size, ph->smoketex[0].xend, ph->smoketex[0].ystart, col);
			va->AddVertexTC(pos1+(-camera->up+camera->right)*size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
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
