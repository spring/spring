#include "System/StdAfx.h"
#include "BeamLaserProjectile.h"
#include "ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Rendering/GL/myGL.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
//#include "System/mmgr.h"

CBeamLaserProjectile::CBeamLaserProjectile(const float3& startPos,const float3& endPos,float startAlpha,float endAlpha,const float3& color,CUnit* owner,float thickness)
: CProjectile((startPos+endPos)*0.5,ZeroVector,owner),
	startPos(startPos),
	endPos(endPos),
	startAlpha(startAlpha),
	endAlpha(endAlpha),
	color(color),
	thickness(thickness)
{
	checkCol=false;
	useAirLos=true;

	SetRadius(pos.distance(endPos));
}

CBeamLaserProjectile::~CBeamLaserProjectile(void)
{
}

void CBeamLaserProjectile::Update(void)
{
	deleteMe=true;
}

void CBeamLaserProjectile::Draw(void)
{
	inArray=true;
	float3 dif(pos-camera->pos);
	float camDist=dif.Length();
	dif/=camDist;
	float3 dir=endPos-startPos;
	dir.Normalize();
	float3 dir1(dif.cross(dir));
	dir1.Normalize();
	float3 dir2(dif.cross(dir1));

	unsigned char col[4];
	col[0]=(unsigned char)(color.x*startAlpha);
	col[1]=(unsigned char)(color.y*startAlpha);
	col[2]=(unsigned char)(color.z*startAlpha);
	col[3]=1;//intensity*255;

	unsigned char col2[4];
	col2[0]=(unsigned char)(color.x*endAlpha);
	col2[1]=(unsigned char)(color.y*endAlpha);
	col2[2]=(unsigned char)(color.z*endAlpha);
	col2[3]=1;//intensity*255;

	float size=thickness;
	float3 pos1=startPos;
	float3 pos2=endPos;

	if(camDist<1000){
		va->AddVertexTC(pos1-dir1*size,					  15.0/16,0,    col);
		va->AddVertexTC(pos1+dir1*size,					  15.0/16,1.0/8,col);
		va->AddVertexTC(pos1+dir1*size-dir2*size, 14.0/16,1.0/8,col);
		va->AddVertexTC(pos1-dir1*size-dir2*size, 14.0/16,0		,col);

		va->AddVertexTC(pos1-dir1*size,11.0/16,0,    col);
		va->AddVertexTC(pos1+dir1*size,11.0/16,1.0/8,col);
		va->AddVertexTC(pos2+dir1*size,11.0/16,1.0/8,col2);
		va->AddVertexTC(pos2-dir1*size,11.0/16,0    ,col2);

		va->AddVertexTC(pos2-dir1*size,					  15.0/16,0,    col2);
		va->AddVertexTC(pos2+dir1*size,					  15.0/16,1.0/8,col2);
		va->AddVertexTC(pos2+dir1*size+dir2*size, 14.0/16,1.0/8,col2);
		va->AddVertexTC(pos2-dir1*size+dir2*size, 14.0/16,0		,col2);
	} else {
		va->AddVertexTC(pos1-dir1*size,11.0/16,0,    col);
		va->AddVertexTC(pos1+dir1*size,11.0/16,1.0/8,col);
		va->AddVertexTC(pos2+dir1*size,11.0/16,1.0/8,col2);
		va->AddVertexTC(pos2-dir1*size,11.0/16,0    ,col2);
	}
}
