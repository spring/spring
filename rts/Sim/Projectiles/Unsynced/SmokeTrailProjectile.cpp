// SmokeTrailProjectile.cpp: implementation of the CSmokeTrailProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "mmgr.h"

#include "SmokeTrailProjectile.h"

#include "Rendering/Textures/TextureAtlas.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "myMath.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CSmokeTrailProjectile, CProjectile, (float3(0,0,0),float3(0,0,0),float3(0,0,0),float3(0,0,0),NULL,0,0,0,0,0,0,NULL,NULL));

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
	CR_MEMBER(drawCallbacker),
	CR_MEMBER(texture),
	CR_RESERVED(4)
	));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSmokeTrailProjectile::CSmokeTrailProjectile(
	const float3& pos1,
	const float3& pos2,
	const float3& dir1,
	const float3& dir2,
	CUnit* owner,
	bool firstSegment,
	bool lastSegment,
	float size,
	float time,
	float color,
	bool drawTrail,
	CProjectile* drawCallback,
	AtlasedTexture* texture
	GML_PARG_C):

	CProjectile((pos1 + pos2) * 0.5f, ZeroVector, owner, false, false, false GML_PARG_P),
	pos1(pos1),
	pos2(pos2),
	orgSize(size),
	creationTime(gs->frameNum),
	lifeTime((int)time),
	color(color),
	dir1(dir1),
	dir2(dir2),
	drawTrail(drawTrail),
	drawSegmented(false),
	firstSegment(firstSegment),
	lastSegment(lastSegment),
	drawCallbacker(drawCallback),
	texture(texture)
{
	checkCol=false;
	castShadow=true;

	//if no custom texture is defined, use the default texture
	//Note that this will crash anyway (no idea why) so never have a null texture!
	if (texture==0) {
		texture = &ph->smoketrailtex;
	}

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
		middir=(dir1+dir2).ANormalize();
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
	va->EnlargeArrays(8*4,0,VA_SIZE_TC);

	if(drawTrail){
		float3 dif(pos1-camera->pos2);
		dif.ANormalize();
		float3 odir1(dif.cross(dir1));
		odir1.ANormalize();
		float3 dif2(pos2-camera->pos2);
		dif2.ANormalize();
		float3 odir2(dif2.cross(dir2));
		odir2.ANormalize();

		unsigned char col[4];
		float a1=(1-float(age)/(lifeTime))*255;
		if(lastSegment)
			a1=0;
		a1*=0.7f+fabs(dif.dot(dir1));
		float alpha=std::min(255.f,std::max(0.f,a1));
		col[0]=(unsigned char) (color*alpha);
		col[1]=(unsigned char) (color*alpha);
		col[2]=(unsigned char) (color*alpha);
		col[3]=(unsigned char)alpha;

		unsigned char col2[4];
		float a2=(1-float(age+8)/(lifeTime))*255;
		if(firstSegment)
			a2=0;
		a2*=0.7f+fabs(dif2.dot(dir2));
		alpha=std::min(255.f,std::max(0.f,a2));
		col2[0]=(unsigned char) (color*alpha);
		col2[1]=(unsigned char) (color*alpha);
		col2[2]=(unsigned char) (color*alpha);
		col2[3]=(unsigned char)alpha;

		float size=1+(age*(1.0f/lifeTime))*orgSize;
		float size2=1+((age+8)*(1.0f/lifeTime))*orgSize;

		if(drawSegmented){
			float3 dif3(midpos-camera->pos2);
			dif3.ANormalize();
			float3 odir3(dif3.cross(middir));
			odir3.ANormalize();
			float size3=0.2f+((age+4)*(1.0f/lifeTime))*orgSize;

			unsigned char col3[4];
			float a2=(1-float(age+4)/(lifeTime))*255;
			a2*=0.7f+fabs(dif3.dot(middir));
			alpha=std::min(255.f,std::max(0.f,a2));
			col3[0]=(unsigned char) (color*alpha);
			col3[1]=(unsigned char) (color*alpha);
			col3[2]=(unsigned char) (color*alpha);
			col3[3]=(unsigned char)alpha;

			float midtexx = texture->xstart + (texture->xend - texture->xstart)*0.5f;

			va->AddVertexQTC(pos1-odir1*size,texture->xstart,texture->ystart,col);
			va->AddVertexQTC(pos1+odir1*size,texture->xstart,texture->yend,col);
			va->AddVertexQTC(midpos+odir3*size3,midtexx,texture->yend,col3);
			va->AddVertexQTC(midpos-odir3*size3,midtexx,texture->ystart,col3);

			va->AddVertexQTC(midpos-odir3*size3,midtexx,texture->ystart,col3);
			va->AddVertexQTC(midpos+odir3*size3,midtexx,texture->yend,col3);
			va->AddVertexQTC(pos2+odir2*size2,texture->xend,texture->yend,col2);
			va->AddVertexQTC(pos2-odir2*size2,texture->xend,texture->ystart,col2);
		} else {
			va->AddVertexQTC(pos1-odir1*size,texture->xstart,texture->ystart,col);
			va->AddVertexQTC(pos1+odir1*size,texture->xstart,texture->yend,col);
			va->AddVertexQTC(pos2+odir2*size2,texture->xend,texture->yend,col2);
			va->AddVertexQTC(pos2-odir2*size2,texture->xend,texture->ystart,col2);
		}
	} else {	//draw as particles
		unsigned char col[4];
		for(int a=0;a<8;++a){
			float a1=1-float(age+a)/lifeTime;
			float alpha=std::min(255.f,std::max(0.f,a1*255));
			col[0]=(unsigned char) (color*alpha);
			col[1]=(unsigned char) (color*alpha);
			col[2]=(unsigned char) (color*alpha);
			col[3]=(unsigned char)alpha;
			float size=((0.2f+(age+a)*(1.0f/lifeTime))*orgSize)*1.2f;

			float3 pos=CalcBeizer(a/8.0f,pos1,dirpos1,dirpos2,pos2);
			va->AddVertexQTC(pos1+( camera->up+camera->right)*size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
			va->AddVertexQTC(pos1+( camera->up-camera->right)*size, ph->smoketex[0].xend, ph->smoketex[0].ystart, col);
			va->AddVertexQTC(pos1+(-camera->up-camera->right)*size, ph->smoketex[0].xend, ph->smoketex[0].ystart, col);
			va->AddVertexQTC(pos1+(-camera->up+camera->right)*size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
		}
	}
#if defined(USE_GML) && GML_ENABLE_SIM
	CProjectile * callbacker = *(CProjectile * volatile *)&drawCallbacker;
	if(callbacker)
		callbacker->DrawCallback();
#else
	if(drawCallbacker)
		drawCallbacker->DrawCallback();
#endif
}

void CSmokeTrailProjectile::Update()
{
	if(gs->frameNum>=creationTime+lifeTime)
		deleteMe=true;
}
