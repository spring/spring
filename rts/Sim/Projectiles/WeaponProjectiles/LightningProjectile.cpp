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

CR_BIND_DERIVED(CLightningProjectile, CWeaponProjectile, (ZeroVector, ZeroVector, NULL, ZeroVector, NULL, 0, NULL));

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
		int ttl, CWeapon* weap)
	: CWeaponProjectile(pos, ZeroVector, owner, NULL, ZeroVector, weaponDef, NULL, ttl)
	, color(color)
	, endPos(end)
	, weapon(weap)
{
	projectileType = WEAPON_LIGHTNING_PROJECTILE;
	checkCol = false;
	drawRadius = pos.distance(endPos);

	displacements[0] = 0.0f;
	for (size_t d = 1; d < displacements_size; ++d) {
		displacements[d]  = (gs->randFloat() - 0.5f) * drawRadius * 0.05f;
	}

	displacements2[0] = 0.0f;
	for (size_t d = 1; d < displacements_size; ++d) {
		displacements2[d] = (gs->randFloat() - 0.5f) * drawRadius * 0.05f;
	}

	if (weapon) {
		AddDeathDependence(weapon);
	}

#ifdef TRACE_SYNC
	tracefile << "New lightning: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << end.x << " " << end.y << " " << end.z << "\n";
#endif

	cegID = gCEG->Load(explGenHandler, cegTag);
}

CLightningProjectile::~CLightningProjectile()
{
}

void CLightningProjectile::Update()
{
	if (--ttl <= 0) {
		deleteMe = true;
	} else {
		gCEG->Explosion(cegID, pos + ((endPos - pos) / ttl), 0.0f, displacements[0], NULL, 0.0f, NULL, endPos - pos);
	}

	if (weapon && !luaMoveCtrl) {
		pos = weapon->weaponMuzzlePos;
	}

	for (size_t d = 1; d < displacements_size; ++d) {
		displacements[d]  += (gs->randFloat() - 0.5f) * 0.3f;
		displacements2[d] += (gs->randFloat() - 0.5f) * 0.3f;
	}
}

void CLightningProjectile::Draw()
{
	inArray = true;
	unsigned char col[4];
	col[0] = (unsigned char) (color.x * 255);
	col[1] = (unsigned char) (color.y * 255);
	col[2] = (unsigned char) (color.z * 255);
	col[3] = 1; //intensity*255;

	const float3 ddir = (endPos - pos).Normalize();
	float3 dif(pos - camera->pos);
	float camDist = dif.Length();
	dif /= camDist;
	const float3 dir1 = (dif.cross(ddir)).Normalize();
	float3 tempPos = pos;

	va->EnlargeArrays(18 * 4, 0, VA_SIZE_TC);
	for (size_t d = 1; d < displacements_size-1; ++d) {
		float f = (d + 1) * 0.111f;

		#define WD weaponDef
		va->AddVertexQTC(tempPos + (dir1 * (displacements[d    ] + WD->thickness)), WD->visuals.texture1->xstart, WD->visuals.texture1->ystart, col);
		va->AddVertexQTC(tempPos + (dir1 * (displacements[d    ] - WD->thickness)), WD->visuals.texture1->xstart, WD->visuals.texture1->yend,   col);
		tempPos = (pos * (1.0f - f)) + (endPos * f);
		va->AddVertexQTC(tempPos + (dir1 * (displacements[d + 1] - WD->thickness)), WD->visuals.texture1->xend,   WD->visuals.texture1->yend,   col);
		va->AddVertexQTC(tempPos + (dir1 * (displacements[d + 1] + WD->thickness)), WD->visuals.texture1->xend,   WD->visuals.texture1->ystart, col);
		#undef WD
	}

	tempPos = pos;
	for (size_t d = 1; d < displacements_size-1; ++d) {
		float f = (d + 1) * 0.111f;

		#define WD weaponDef
		va->AddVertexQTC(tempPos + dir1 * (displacements2[d    ] + WD->thickness), WD->visuals.texture1->xstart, WD->visuals.texture1->ystart, col);
		va->AddVertexQTC(tempPos + dir1 * (displacements2[d    ] - WD->thickness), WD->visuals.texture1->xstart, WD->visuals.texture1->yend,   col);
		tempPos = pos * (1.0f - f) + endPos * f;
		va->AddVertexQTC(tempPos + dir1 * (displacements2[d + 1] - WD->thickness), WD->visuals.texture1->xend,   WD->visuals.texture1->yend,   col);
		va->AddVertexQTC(tempPos + dir1 * (displacements2[d + 1] + WD->thickness), WD->visuals.texture1->xend,   WD->visuals.texture1->ystart, col);
		#undef WD
	}
}

void CLightningProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	const unsigned char lcolor[4] = {
			(unsigned char)color[0] * 255,
			(unsigned char)color[1] * 255,
			(unsigned char)color[2] * 255,
			1                       * 255
			};
	lines.AddVertexQC(pos,    lcolor);
	lines.AddVertexQC(endPos, lcolor);
}

void CLightningProjectile::DependentDied(CObject* o)
{
	if (o == weapon) {
		weapon = NULL;
	}
}
