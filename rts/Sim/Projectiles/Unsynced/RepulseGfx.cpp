#include "StdAfx.h"
#include "mmgr.h"

#include "RepulseGfx.h"
#include "Sim/Units/Unit.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CRepulseGfx, CProjectile, (NULL,NULL,0,float3(0,0,0)));

CR_REG_METADATA(CRepulseGfx,(
	CR_MEMBER(repulsed),
	CR_MEMBER(sqMaxDist),
	CR_MEMBER(age),
	CR_MEMBER(color),
	CR_MEMBER(difs),
	CR_RESERVED(8)
	));

CRepulseGfx::CRepulseGfx(CUnit* owner, CProjectile* repulsed, float maxDist, float3 color GML_PARG_C):
	CProjectile(repulsed? repulsed->pos: ZeroVector, repulsed? repulsed->speed: ZeroVector, owner, false, false, false GML_PARG_P),
	repulsed(repulsed),
	sqMaxDist((maxDist * maxDist) + 100),
	age(0),
	color(color)
{
	if (repulsed)
		AddDeathDependence(repulsed);

	checkCol = false;
	useAirLos = true;
	SetRadius(maxDist);

	for (int y = 0; y < 5; ++y) {
		float yp = (y / 4.0f - 0.5f);

		for (int x = 0; x < 5; ++x) {
			float xp = (x / 4.0f - 0.5f);
			float d = fastmath::sqrt2(xp * xp + yp * yp);
			difs[y * 5 + x] = (1 - fastmath::cos(d * 2)) * 20;
		}
	}
}

CRepulseGfx::~CRepulseGfx(void)
{
}

void CRepulseGfx::DependentDied(CObject* o)
{
	if(o==repulsed){
		repulsed=0;
		deleteMe=true;
	}
}

void CRepulseGfx::Draw(void)
{
	CUnit* owner = CProjectile::owner();
	if(!owner || !repulsed)
		return;

	float3 dir=repulsed->pos-owner->pos;
	dir.ANormalize();

	pos=repulsed->pos-dir*10+repulsed->speed*gu->timeOffset;

	float3 dir1=dir.cross(UpVector);
	dir1.ANormalize();
	float3 dir2=dir1.cross(dir);

	inArray=true;
	unsigned char col[4];
	float alpha=std::min(255,age*10);
	col[0]=(unsigned char)(color.x*alpha);
	col[1]=(unsigned char)(color.y*alpha);
	col[2]=(unsigned char)(color.z*alpha);
	col[3]=(unsigned char)(alpha*0.2f);
	float drawsize=10;

	AtlasedTexture& et=ph->repulsetex;
	float txo=et.xstart;
	float tyo=et.ystart;
	float txs=et.xend-et.xstart;
	float tys=et.yend-et.ystart;

	va->EnlargeArrays(4*4*4+16,0,VA_SIZE_TC);
	for(int y=0;y<4;++y){ //! CAUTION: loop count must match EnlargeArrays above
		float dy=y-2;
		float ry=y*0.25f;
		for(int x=0;x<4;++x){
			float dx=x-2;
			float rx=x*0.25f;
			va->AddVertexQTC(pos+dir1*drawsize*(dx+0)+dir2*drawsize*(dy+0)+dir*difs[y*5+x]			 ,txo+ry*txs				,tyo+(rx)*tys			,col);
			va->AddVertexQTC(pos+dir1*drawsize*(dx+0)+dir2*drawsize*(dy+1)+dir*difs[(y+1)*5+x]  ,txo+(ry+0.25f)*txs	,tyo+(rx)*tys			,col);
			va->AddVertexQTC(pos+dir1*drawsize*(dx+1)+dir2*drawsize*(dy+1)+dir*difs[(y+1)*5+x+1],txo+(ry+0.25f)*txs	,tyo+(rx+0.25f)*tys,col);
			va->AddVertexQTC(pos+dir1*drawsize*(dx+1)+dir2*drawsize*(dy+0)+dir*difs[y*5+x+1]		 ,txo+ry*txs				,tyo+(rx+0.25f)*tys,col);
		}
	}
	drawsize=7;
	alpha=std::min(10,age/2);
	col[0]=(unsigned char)(color.x*alpha);
	col[1]=(unsigned char)(color.y*alpha);
	col[2]=(unsigned char)(color.z*alpha);
	col[3]=(unsigned char)(alpha*0.4f);

	AtlasedTexture& ct=ph->repulsegfxtex;
	float tx=(ct.xend+ct.xstart)*0.5f;
	float ty=(ct.yend+ct.ystart)*0.5f;

	unsigned char col2[4];
	col2[0]=0;
	col2[1]=0;
	col2[2]=0;
	col2[3]=0;

	va->AddVertexQTC(owner->pos+(-dir1+dir2)*drawsize*0.2f,tx,ty,col2);
	va->AddVertexQTC(owner->pos+(dir1+dir2)*drawsize*0.2f,tx,ty,col2);
	va->AddVertexQTC(pos+dir1*drawsize+dir2*drawsize+dir*difs[6],tx,ty,col);
	va->AddVertexQTC(pos-dir1*drawsize+dir2*drawsize+dir*difs[6],tx,ty,col);

	va->AddVertexQTC(owner->pos+(-dir1-dir2)*drawsize*0.2f,tx,ty,col2);
	va->AddVertexQTC(owner->pos+(dir1-dir2)*drawsize*0.2f,tx,ty,col2);
	va->AddVertexQTC(pos+dir1*drawsize-dir2*drawsize+dir*difs[6],tx,ty,col);
	va->AddVertexQTC(pos-dir1*drawsize-dir2*drawsize+dir*difs[6],tx,ty,col);

	va->AddVertexQTC(owner->pos+(dir1-dir2)*drawsize*0.2f,tx,ty,col2);
	va->AddVertexQTC(owner->pos+(dir1+dir2)*drawsize*0.2f,tx,ty,col2);
	va->AddVertexQTC(pos+dir1*drawsize+dir2*drawsize+dir*difs[6],tx,ty,col);
	va->AddVertexQTC(pos+dir1*drawsize-dir2*drawsize+dir*difs[6],tx,ty,col);

	va->AddVertexQTC(owner->pos+(-dir1-dir2)*drawsize*0.2f,tx,ty,col2);
	va->AddVertexQTC(owner->pos+(-dir1+dir2)*drawsize*0.2f,tx,ty,col2);
	va->AddVertexQTC(pos-dir1*drawsize+dir2*drawsize+dir*difs[6],tx,ty,col);
	va->AddVertexQTC(pos-dir1*drawsize-dir2*drawsize+dir*difs[6],tx,ty,col);
}

void CRepulseGfx::Update(void)
{
	age++;

	if(repulsed && owner() && (repulsed->pos-owner()->pos).SqLength()>sqMaxDist)
		deleteMe=true;
}
