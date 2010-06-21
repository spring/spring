/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "LightningProjectile.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

#ifdef TRACE_SYNC
	#include "Sync/SyncTracer.h"
#endif

CR_BIND_DERIVED(CLightningProjectile, CWeaponProjectile, (float3(0,0,0),float3(0,0,0),NULL,float3(0,0,0),NULL,0,NULL));

CR_REG_METADATA(CLightningProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(color),
	CR_MEMBER(endPos),
	CR_MEMBER(weapon),
	CR_MEMBER(displacements),
	CR_MEMBER(displacements2),
	CR_RESERVED(16)
	));

CLightningProjectile::CLightningProjectile(
	const float3& pos, const float3& end,
	CUnit* owner,
	const float3& color,
	const WeaponDef* weaponDef,
	int ttl, CWeapon* weap):

	CWeaponProjectile(pos, ZeroVector, owner, 0, ZeroVector, weaponDef, 0, ttl),
	color(color),
	endPos(end),
	weapon(weap)
{
	projectileType = WEAPON_LIGHTNING_PROJECTILE;
	checkCol = false;
	drawRadius = pos.distance(endPos);

	displacements[0]=0;
	for(int a=1;a<10;++a)
		displacements[a]=(gs->randFloat()-0.5f)*drawRadius*0.05f;

	displacements2[0]=0;
	for(int a=1;a<10;++a)
		displacements2[a]=(gs->randFloat()-0.5f)*drawRadius*0.05f;

	if(weapon)
		AddDeathDependence(weapon);

#ifdef TRACE_SYNC
	tracefile << "New lightning: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << end.x << " " << end.y << " " << end.z << "\n";
#endif

	if (!cegTag.empty()) {
		ceg.Load(explGenHandler, cegTag);
	}
}

CLightningProjectile::~CLightningProjectile(void)
{
}

void CLightningProjectile::Update(void)
{
	if (--ttl <= 0) {
		deleteMe = true;
	} else {
		if (!cegTag.empty()) {
			ceg.Explosion(pos + ((endPos - pos) / ttl), 0.0f, displacements[0], 0x0, 0.0f, 0x0, endPos - pos);
		}
	}

	if (weapon && !luaMoveCtrl) {
		pos = weapon->weaponMuzzlePos;
	}

	for (int a = 1; a < 10; ++a) {
		displacements[a] += (gs->randFloat() - 0.5f) * 0.3f;
		displacements2[a] += (gs->randFloat() - 0.5f) * 0.3f;
	}
}

void CLightningProjectile::Draw(void)
{
	inArray=true;
	unsigned char col[4];
	col[0]=(unsigned char) (color.x*255);
	col[1]=(unsigned char) (color.y*255);
	col[2]=(unsigned char) (color.z*255);
	col[3]=1;//intensity*255;

	const float3 ddir = (endPos - pos).Normalize();
	float3 dif(pos - camera->pos);
	float camDist = dif.Length();
	dif /= camDist;
	const float3 dir1 = (dif.cross(ddir)).Normalize();
	float3 tempPos = pos;

	va->EnlargeArrays(18 * 4, 0, VA_SIZE_TC);
	for (int a = 0; a < 9; ++a) {
		float f = (a + 1) * 0.111f;

		#define WD weaponDef
		va->AddVertexQTC(tempPos + dir1 * (displacements[a    ] + WD->thickness), WD->visuals.texture1->xstart, WD->visuals.texture1->ystart, col);
		va->AddVertexQTC(tempPos + dir1 * (displacements[a    ] - WD->thickness), WD->visuals.texture1->xstart, WD->visuals.texture1->yend,   col);
		tempPos = pos * (1.0f - f) + endPos * f;
		va->AddVertexQTC(tempPos + dir1 * (displacements[a + 1] - WD->thickness), WD->visuals.texture1->xend,   WD->visuals.texture1->yend,   col);
		va->AddVertexQTC(tempPos + dir1 * (displacements[a + 1] + WD->thickness), WD->visuals.texture1->xend,   WD->visuals.texture1->ystart, col);
		#undef WD
	}

	tempPos = pos;
	for (int a = 0; a < 9; ++a) {
		float f = (a + 1) * 0.111f;

		#define WD weaponDef
		va->AddVertexQTC(tempPos + dir1 * (displacements2[a    ] + WD->thickness), WD->visuals.texture1->xstart, WD->visuals.texture1->ystart, col);
		va->AddVertexQTC(tempPos + dir1 * (displacements2[a    ] - WD->thickness), WD->visuals.texture1->xstart, WD->visuals.texture1->yend,   col);
		tempPos = pos * (1.0f - f) + endPos * f;
		va->AddVertexQTC(tempPos + dir1 * (displacements2[a + 1] - WD->thickness), WD->visuals.texture1->xend, WD->visuals.texture1->yend,   col);
		va->AddVertexQTC(tempPos + dir1 * (displacements2[a + 1] + WD->thickness), WD->visuals.texture1->xend, WD->visuals.texture1->ystart, col);
		#undef WD
	}
}

void CLightningProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	unsigned char lcolor[4] = {(unsigned char)color[0]*255,(unsigned char)color[1]*255,(unsigned char)color[2]*255,255};
	lines.AddVertexQC(pos, lcolor);
	lines.AddVertexQC(endPos, lcolor);
}

void CLightningProjectile::DependentDied(CObject* o)
{
	if (o == weapon)
		weapon = 0;
}
