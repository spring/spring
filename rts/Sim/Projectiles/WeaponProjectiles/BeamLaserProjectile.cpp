/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "BeamLaserProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

CR_BIND_DERIVED(CBeamLaserProjectile, CWeaponProjectile,
	(float3(ZeroVector), float3(ZeroVector), 0.0f, 0.0f, float3(ZeroVector), NULL, NULL));

CR_REG_METADATA(CBeamLaserProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(startPos),
	CR_MEMBER(endPos),
	CR_MEMBER(corecolstart),
	CR_MEMBER(corecolend),
	CR_MEMBER(kocolstart),
	CR_MEMBER(kocolend),
	CR_MEMBER(thickness),
	CR_MEMBER(corethickness),
	CR_MEMBER(flaresize),
	CR_MEMBER(decay),
	CR_MEMBER(midtexx),
	CR_RESERVED(16)
	));

CBeamLaserProjectile::CBeamLaserProjectile(
	const float3& startPos, const float3& endPos,
	float startAlpha, float endAlpha,
	const float3& color,
	CUnit* owner,
	const WeaponDef* weaponDef):

	CWeaponProjectile((startPos + endPos) * 0.5f, ZeroVector, owner, NULL, ZeroVector, weaponDef, NULL, (weaponDef? weaponDef->visuals.beamttl: 0)),
	startPos(startPos),
	endPos(endPos),
	thickness(weaponDef? weaponDef->thickness: 0.0f),
	corethickness(weaponDef? weaponDef->corethickness: 0.0f),
	flaresize(weaponDef? weaponDef->laserflaresize: 0.0f),
	decay(weaponDef? weaponDef->visuals.beamdecay: 0.0f)
{
	projectileType = WEAPON_BEAMLASER_PROJECTILE;
	checkCol = false;
	useAirLos = true;

	SetRadius(pos.distance(endPos));

	if (weaponDef) {
		midtexx =
			(weaponDef->visuals.texture2->xstart +
			(weaponDef->visuals.texture2->xend - weaponDef->visuals.texture2->xstart) * 0.5f);

		corecolstart[0] = (weaponDef->visuals.color2.x * startAlpha);
		corecolstart[1] = (weaponDef->visuals.color2.y * startAlpha);
		corecolstart[2] = (weaponDef->visuals.color2.z * startAlpha);
		corecolstart[3] = 1;
		corecolend[0] = (weaponDef->visuals.color2.x * endAlpha);
		corecolend[1] = (weaponDef->visuals.color2.y * endAlpha);
		corecolend[2] = (weaponDef->visuals.color2.z * endAlpha);
		corecolend[3] = 1;
		kocolstart[0] = (color.x * startAlpha);
		kocolstart[1] = (color.y * startAlpha);
		kocolstart[2] = (color.z * startAlpha);
		kocolstart[3] = 1;
		kocolend[0] = (color.x * endAlpha);
		kocolend[1] = (color.y * endAlpha);
		kocolend[2] = (color.z * endAlpha);
		kocolend[3] = 1;
	} else {
		midtexx = 0.0f;
	}

	cegID = gCEG->Load(explGenHandler, cegTag);
}



void CBeamLaserProjectile::Update()
{
	if (ttl <= 0) {
		deleteMe = true;
	} else {
		ttl--;
		for (int i = 0; i < 3; i++) {
			corecolstart[i] = (corecolstart[i] * decay);
			corecolend[i] = (corecolend[i] * decay);
			kocolstart[i] = (kocolstart[i] * decay);
			kocolend[i] = (kocolend[i] * decay);
		}

		gCEG->Explosion(cegID, startPos + ((endPos - startPos) / ttl), 0.0f, flaresize, 0x0, 0.0f, 0x0, endPos - startPos);
	}
}

void CBeamLaserProjectile::Draw()
{
	inArray = true;

	float3 dif(pos - camera->pos);
	float camDist = dif.Length();
	dif /= camDist;

	const float3 ddir = (endPos - startPos).Normalize();
	const float3 dir1 = (dif.cross(ddir)).Normalize();
	const float3 dir2(dif.cross(dir1));

	const float size = thickness;
	const float coresize = size * corethickness;
	const float3& pos1 = startPos;
	const float3& pos2 = endPos;

	va->EnlargeArrays(32, 0, VA_SIZE_TC);

	#define WT1 weaponDef->visuals.texture1
	#define WT2 weaponDef->visuals.texture2
	#define WT3 weaponDef->visuals.texture3

	if (camDist < 1000.0f) {
		va->AddVertexQTC(pos1 - dir1 * size,                       midtexx,   WT2->ystart, kocolstart);
		va->AddVertexQTC(pos1 + dir1 * size,                       midtexx,   WT2->yend,   kocolstart);
		va->AddVertexQTC(pos1 + dir1 * size - dir2 * size,         WT2->xend, WT2->yend,   kocolstart);
		va->AddVertexQTC(pos1 - dir1 * size - dir2 * size,         WT2->xend, WT2->ystart, kocolstart);
		va->AddVertexQTC(pos1 - dir1 * coresize,                   midtexx,   WT2->ystart, corecolstart);
		va->AddVertexQTC(pos1 + dir1 * coresize,                   midtexx,   WT2->yend,   corecolstart);
		va->AddVertexQTC(pos1 + dir1 * coresize - dir2 * coresize, WT2->xend, WT2->yend,   corecolstart);
		va->AddVertexQTC(pos1 - dir1 * coresize - dir2 * coresize, WT2->xend, WT2->ystart, corecolstart);

		va->AddVertexQTC(pos1 - dir1 * size,                       WT1->xstart, WT1->ystart, kocolstart);
		va->AddVertexQTC(pos1 + dir1 * size,                       WT1->xstart, WT1->yend,   kocolstart);
		va->AddVertexQTC(pos2 + dir1 * size,                       WT1->xend,   WT1->yend,   kocolend);
		va->AddVertexQTC(pos2 - dir1 * size,                       WT1->xend,   WT1->ystart, kocolend);
		va->AddVertexQTC(pos1 - dir1 * coresize,                   WT1->xstart, WT1->ystart, corecolstart);
		va->AddVertexQTC(pos1 + dir1 * coresize,                   WT1->xstart, WT1->yend,   corecolstart);
		va->AddVertexQTC(pos2 + dir1 * coresize,                   WT1->xend,   WT1->yend,   corecolend);
		va->AddVertexQTC(pos2 - dir1 * coresize,                   WT1->xend,   WT1->ystart, corecolend);

		va->AddVertexQTC(pos2 - dir1 * size,                       midtexx,   WT2->ystart, kocolstart);
		va->AddVertexQTC(pos2 + dir1 * size,                       midtexx,   WT2->yend,   kocolstart);
		va->AddVertexQTC(pos2 + dir1 * size + dir2 * size,         WT2->xend, WT2->yend,   kocolstart);
		va->AddVertexQTC(pos2 - dir1 * size + dir2 * size,         WT2->xend, WT2->ystart, kocolstart);
		va->AddVertexQTC(pos2 - dir1 * coresize,                   midtexx,   WT2->ystart, corecolstart);
		va->AddVertexQTC(pos2 + dir1 * coresize,                   midtexx,   WT2->yend,   corecolstart);
		va->AddVertexQTC(pos2 + dir1 * coresize + dir2 * coresize, WT2->xend, WT2->yend,   corecolstart);
		va->AddVertexQTC(pos2 - dir1 * coresize + dir2 * coresize, WT2->xend, WT2->ystart, corecolstart);
	} else {
		va->AddVertexQTC(pos1 - dir1 * size,                       WT1->xstart, WT1->ystart, kocolstart);
		va->AddVertexQTC(pos1 + dir1 * size,                       WT1->xstart, WT1->yend,   kocolstart);
		va->AddVertexQTC(pos2 + dir1 * size,                       WT1->xend,   WT1->yend,   kocolend);
		va->AddVertexQTC(pos2 - dir1 * size,                       WT1->xend,   WT1->ystart, kocolend);
		va->AddVertexQTC(pos1 - dir1 * coresize,                   WT1->xstart, WT1->ystart, corecolstart);
		va->AddVertexQTC(pos1 + dir1 * coresize,                   WT1->xstart, WT1->yend,   corecolstart);
		va->AddVertexQTC(pos2 + dir1 * coresize,                   WT1->xend,   WT1->yend,   corecolend);
		va->AddVertexQTC(pos2 - dir1 * coresize,                   WT1->xend,   WT1->ystart, corecolend);
	}

	// draw flare
	float fsize = size * flaresize;
	va->AddVertexQTC(pos1 - camera->right * fsize-camera->up * fsize, WT3->xstart, WT3->ystart, kocolstart);
	va->AddVertexQTC(pos1 + camera->right * fsize-camera->up * fsize, WT3->xend,   WT3->ystart, kocolstart);
	va->AddVertexQTC(pos1 + camera->right * fsize+camera->up * fsize, WT3->xend,   WT3->yend,   kocolstart);
	va->AddVertexQTC(pos1 - camera->right * fsize+camera->up * fsize, WT3->xstart, WT3->yend,   kocolstart);

	fsize = fsize * corethickness;
	va->AddVertexQTC(pos1 - camera->right * fsize-camera->up * fsize, WT3->xstart, WT3->ystart, corecolstart);
	va->AddVertexQTC(pos1 + camera->right * fsize-camera->up * fsize, WT3->xend,   WT3->ystart, corecolstart);
	va->AddVertexQTC(pos1 + camera->right * fsize+camera->up * fsize, WT3->xend,   WT3->yend,   corecolstart);
	va->AddVertexQTC(pos1 - camera->right * fsize+camera->up * fsize, WT3->xstart, WT3->yend,   corecolstart);

	#undef WT3
	#undef WT2
	#undef WT1
}

void CBeamLaserProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	unsigned char color[4] = { kocolstart[0], kocolstart[1], kocolstart[2], 255 };
	lines.AddVertexQC(startPos, color);
	lines.AddVertexQC(endPos, color);
}
