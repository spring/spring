#include "StdAfx.h"
#include "LightingProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Weapons/Weapon.h"
#include "mmgr.h"
#include "ProjectileHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"

CLightingProjectile::CLightingProjectile(const float3& pos,const float3& end,CUnit* owner,const float3& color, WeaponDef *weaponDef,int ttl,CWeapon* weap)
:	CWeaponProjectile(pos,ZeroVector, owner, 0, ZeroVector, weaponDef,damages,0), //CProjectile(pos,ZeroVector,owner),
	ttl(ttl),
	color(color),
	endPos(end),
	weapon(weap)
{
	checkCol=false;
	drawRadius=pos.distance(endPos);

	displacements[0]=0;
	for(int a=1;a<10;++a)
		displacements[a]=(gs->randFloat()-0.5)*drawRadius*0.05;

	displacements2[0]=0;
	for(int a=1;a<10;++a)
		displacements2[a]=(gs->randFloat()-0.5)*drawRadius*0.05;

	if(weapon)
		AddDeathDependence(weapon);

#ifdef TRACE_SYNC
	tracefile << "New lighting: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << end.x << " " << end.y << " " << end.z << "\n";
#endif
}

CLightingProjectile::~CLightingProjectile(void)
{
}

void CLightingProjectile::Update(void)
{
	ttl--;

	if(ttl<0){
		deleteMe=true;
	}

	if(weapon){
		pos=weapon->weaponPos;
	}
	for(int a=1;a<10;++a)
		displacements[a]+=(gs->randFloat()-0.5)*0.3;
	for(int a=1;a<10;++a)
		displacements2[a]+=(gs->randFloat()-0.5)*0.3;
}

void CLightingProjectile::Draw(void)
{
	inArray=true;
	unsigned char col[4];
	col[0]=(unsigned char) (color.x*255);
	col[1]=(unsigned char) (color.y*255);
	col[2]=(unsigned char) (color.z*255);
	col[3]=1;//intensity*255;

	float3 dir=(endPos-pos).Normalize();
	float3 dif(pos-camera->pos);
	float camDist=dif.Length();
	dif/=camDist;
	float3 dir1(dif.cross(dir));
	dir1.Normalize();
	float3 tempPos=pos;

	for(int a=0;a<9;++a){
		float f=(a+1)*0.111;
		va->AddVertexTC(tempPos+dir1*(displacements[a]+weaponDef->thickness),weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->ystart,    col);
		va->AddVertexTC(tempPos+dir1*(displacements[a]-weaponDef->thickness),weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->yend,col);
		tempPos=pos*(1-f)+endPos*f;
		va->AddVertexTC(tempPos+dir1*(displacements[a+1]-weaponDef->thickness),weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->yend,col);
		va->AddVertexTC(tempPos+dir1*(displacements[a+1]+weaponDef->thickness),weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->ystart    ,col);
	}

	tempPos=pos;
	for(int a=0;a<9;++a){
		float f=(a+1)*0.111;
		va->AddVertexTC(tempPos+dir1*(displacements2[a]+weaponDef->thickness),weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->ystart,    col);
		va->AddVertexTC(tempPos+dir1*(displacements2[a]-weaponDef->thickness),weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->yend,col);
		tempPos=pos*(1-f)+endPos*f;
		va->AddVertexTC(tempPos+dir1*(displacements2[a+1]-weaponDef->thickness),weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->yend,col);
		va->AddVertexTC(tempPos+dir1*(displacements2[a+1]+weaponDef->thickness),weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->ystart    ,col);
	}
}

void CLightingProjectile::DependentDied(CObject* o)
{
	if(o==weapon)
		weapon=0;
	CProjectile::DependentDied(o);
}
