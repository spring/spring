/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "Rendering/GL/myGL.h"
#include "Game/Camera.h"
#include "LargeBeamLaserProjectile.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/GlobalUnsynced.h"

CR_BIND_DERIVED(CLargeBeamLaserProjectile, CWeaponProjectile, (ZeroVector, ZeroVector, ZeroVector, ZeroVector, NULL, NULL));

CR_REG_METADATA(CLargeBeamLaserProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(startPos),
	CR_MEMBER(endPos),
	CR_MEMBER(corecolstart),
	CR_MEMBER(kocolstart),
	CR_MEMBER(thickness),
	CR_MEMBER(corethickness),
	CR_MEMBER(flaresize),
	CR_MEMBER(tilelength),
	CR_MEMBER(scrollspeed),
	CR_MEMBER(pulseSpeed),
	CR_MEMBER(beamtex),
	CR_MEMBER(side),
	CR_RESERVED(16)
	));

CLargeBeamLaserProjectile::CLargeBeamLaserProjectile(
		const float3& startPos, const float3& endPos,
		const float3& color, const float3& color2,
		CUnit* owner, const WeaponDef* weaponDef)
	: CWeaponProjectile(startPos + (endPos - startPos) * 0.5f, ZeroVector, owner, NULL, ZeroVector, weaponDef, NULL, 1)
	, startPos(startPos)
	, endPos(endPos)
	, decay(1.0f)
{
	projectileType = WEAPON_LARGEBEAMLASER_PROJECTILE;
	checkCol = false;
	useAirLos = true;

	if (weaponDef) {
		this->beamtex = *weaponDef->visuals.texture1;
		this->side    = *weaponDef->visuals.texture3;
	}

	SetRadius(pos.distance(endPos));

	corecolstart[0] = (unsigned char)(color2.x * 255);
	corecolstart[1] = (unsigned char)(color2.y * 255);
	corecolstart[2] = (unsigned char)(color2.z * 255);
	corecolstart[3] = 1;
	kocolstart[0]   = (unsigned char)(color.x * 255);
	kocolstart[1]   = (unsigned char)(color.y * 255);
	kocolstart[2]   = (unsigned char)(color.z * 255);
	kocolstart[3]   = 1;

	if (weaponDef) {
		thickness     = weaponDef->thickness;
		corethickness = weaponDef->corethickness;
		flaresize     = weaponDef->laserflaresize;
		tilelength    = weaponDef->visuals.tilelength;
		scrollspeed   = weaponDef->visuals.scrollspeed;
		pulseSpeed    = weaponDef->visuals.pulseSpeed;
		ttl           = weaponDef->visuals.beamttl;
		decay         = weaponDef->visuals.beamdecay;
	}

	cegID = gCEG->Load(explGenHandler, cegTag);
}



void CLargeBeamLaserProjectile::Update()
{
	if (ttl > 0) {
		ttl--;
		for (int i = 0; i < 3; i++) {
			corecolstart[i] = (unsigned char) (corecolstart[i] * decay);
			kocolstart[i] = (unsigned char) (kocolstart[i] * decay);
		}

		gCEG->Explosion(cegID, startPos + ((endPos - startPos) / ttl), 0.0f, flaresize, NULL, 0.0f, NULL, endPos - startPos);
	}
	else {
		deleteMe = true;
	}
}

void CLargeBeamLaserProjectile::Draw()
{
	inArray = true;

	float3 dif(pos - camera->pos);
	float camDist = dif.Length();
	dif /= camDist;

	float3 ddir(endPos - startPos);
	float beamlength = ddir.Length();
	ddir = ddir / beamlength;

	const float3 dir1((dif.cross(ddir)).Normalize());
	const float3 dir2(dif.cross(dir1));

	float3 pos1 = startPos;
	float3 pos2 = endPos;

	float starttex = (gu->modGameTime) * scrollspeed;
	starttex = 1.0f - (starttex - (int)starttex);
	const float texxsize = beamtex.xend - beamtex.xstart;
	AtlasedTexture tex = beamtex;
	const float& size = thickness;
	const float& coresize = size * corethickness;

	float polylength = (tex.xend-beamtex.xstart)*(1/texxsize)*tilelength;

	float istart = polylength * (1-starttex);
	float iend = beamlength - tilelength;
	va->EnlargeArrays(64 + (8 * ((int)((iend - istart) / tilelength) + 2)), 0, VA_SIZE_TC);
	if (istart > beamlength) {
		// beam short enough to be drawn by one polygon
		// draw laser start
		tex.xstart = beamtex.xstart + starttex*((beamtex.xend-beamtex.xstart));

		va->AddVertexQTC(pos1 - (dir1 * size),     tex.xstart, tex.ystart, kocolstart);
		va->AddVertexQTC(pos1 + (dir1 * size),     tex.xstart, tex.yend,   kocolstart);
		va->AddVertexQTC(pos2 + (dir1 * size),     tex.xend,   tex.yend,   kocolstart);
		va->AddVertexQTC(pos2 - (dir1 * size),     tex.xend,   tex.ystart, kocolstart);
		va->AddVertexQTC(pos1 - (dir1 * coresize), tex.xstart, tex.ystart, corecolstart);
		va->AddVertexQTC(pos1 + (dir1 * coresize), tex.xstart, tex.yend,   corecolstart);
		va->AddVertexQTC(pos2 + (dir1 * coresize), tex.xend,   tex.yend,   corecolstart);
		va->AddVertexQTC(pos2 - (dir1 * coresize), tex.xend,   tex.ystart, corecolstart);
	} else {
		// beam longer than one polygon
		pos2 = pos1 + ddir * istart;

		// draw laser start
		tex.xstart = beamtex.xstart + (starttex * (beamtex.xend - beamtex.xstart));

		va->AddVertexQTC(pos1 - (dir1 * size),     tex.xstart, tex.ystart, kocolstart);
		va->AddVertexQTC(pos1 + (dir1 * size),     tex.xstart, tex.yend,   kocolstart);
		va->AddVertexQTC(pos2 + (dir1 * size),     tex.xend,   tex.yend,   kocolstart);
		va->AddVertexQTC(pos2 - (dir1 * size),     tex.xend,   tex.ystart, kocolstart);
		va->AddVertexQTC(pos1 - (dir1 * coresize), tex.xstart, tex.ystart, corecolstart);
		va->AddVertexQTC(pos1 + (dir1 * coresize), tex.xstart, tex.yend,   corecolstart);
		va->AddVertexQTC(pos2 + (dir1 * coresize), tex.xend,   tex.yend,   corecolstart);
		va->AddVertexQTC(pos2 - (dir1 * coresize), tex.xend,   tex.ystart, corecolstart);

		// draw continous beam
		tex.xstart = beamtex.xstart;
		float i;
		for (i = istart; i < iend; i += tilelength) //! CAUTION: loop count must match EnlargeArrays above
		{
			pos1 = startPos + ddir * i;
			pos2 = startPos + ddir * (i + tilelength);

			va->AddVertexQTC(pos1 - (dir1 * size),     tex.xstart, tex.ystart, kocolstart);
			va->AddVertexQTC(pos1 + (dir1 * size),     tex.xstart, tex.yend,   kocolstart);
			va->AddVertexQTC(pos2 + (dir1 * size),     tex.xend,   tex.yend,   kocolstart);
			va->AddVertexQTC(pos2 - (dir1 * size),     tex.xend,   tex.ystart, kocolstart);
			va->AddVertexQTC(pos1 - (dir1 * coresize), tex.xstart, tex.ystart, corecolstart);
			va->AddVertexQTC(pos1 + (dir1 * coresize), tex.xstart, tex.yend,   corecolstart);
			va->AddVertexQTC(pos2 + (dir1 * coresize), tex.xend,   tex.yend,   corecolstart);
			va->AddVertexQTC(pos2 - (dir1 * coresize), tex.xend,   tex.ystart, corecolstart);
		}

		// draw laser end
		pos1 = startPos + ddir * i;
		pos2 = endPos;
		tex.xend = tex.xstart + ((pos2 - pos1).Length() / tilelength) * texxsize;

		va->AddVertexQTC(pos1 - dir1 * size,     tex.xstart, tex.ystart, kocolstart);
		va->AddVertexQTC(pos1 + dir1 * size,     tex.xstart, tex.yend,   kocolstart);
		va->AddVertexQTC(pos2 + dir1 * size,     tex.xend,   tex.yend,   kocolstart);
		va->AddVertexQTC(pos2 - dir1 * size,     tex.xend,   tex.ystart, kocolstart);
		va->AddVertexQTC(pos1 - dir1 * coresize, tex.xstart, tex.ystart, corecolstart);
		va->AddVertexQTC(pos1 + dir1 * coresize, tex.xstart, tex.yend,   corecolstart);
		va->AddVertexQTC(pos2 + dir1 * coresize, tex.xend,   tex.yend,   corecolstart);
		va->AddVertexQTC(pos2 - dir1 * coresize, tex.xend,   tex.ystart, corecolstart);
	}

	//float midtexx = weaponDef->visuals.texture2->xstart + (weaponDef->visuals.texture2->xend-weaponDef->visuals.texture2->xstart)*0.5f;
	va->AddVertexQTC(pos2 - (dir1 * size),                         weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->ystart, kocolstart);
	va->AddVertexQTC(pos2 + (dir1 * size),                         weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->yend,   kocolstart);
	va->AddVertexQTC(pos2 + (dir1 * size) + (dir2 * size),         weaponDef->visuals.texture2->xend,   weaponDef->visuals.texture2->yend,   kocolstart);
	va->AddVertexQTC(pos2 - (dir1 * size) + (dir2 * size),         weaponDef->visuals.texture2->xend,   weaponDef->visuals.texture2->ystart, kocolstart);
	va->AddVertexQTC(pos2 - (dir1 * coresize),                     weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->ystart, corecolstart);
	va->AddVertexQTC(pos2 + (dir1 * coresize),                     weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->yend,   corecolstart);
	va->AddVertexQTC(pos2 + (dir1 * coresize) + (dir2 * coresize), weaponDef->visuals.texture2->xend,   weaponDef->visuals.texture2->yend,   corecolstart);
	va->AddVertexQTC(pos2 - (dir1 * coresize) + (dir2 * coresize), weaponDef->visuals.texture2->xend,   weaponDef->visuals.texture2->ystart, corecolstart);

	// draw muzzleflare
	starttex  = gu->modGameTime * pulseSpeed;
	starttex -= (int)starttex;

		float muzzlesize = size*flaresize*starttex;
		unsigned char corcol[4];
		unsigned char kocol[4];
		corcol[3] = 1;
		kocol[3] = 1;
		for (int i = 0; i < 3; i++) {
			corcol[i] = (int)(corecolstart[i] * (1 - starttex));
			kocol[i]  = (int)(kocolstart[i]   * (1 - starttex));
		}

		pos1 = startPos - ddir * (size * flaresize) * 0.02f;

		va->AddVertexQTC(pos1 + (dir1 * muzzlesize),                       side.xstart, side.ystart, kocol);
		va->AddVertexQTC(pos1 + (dir1 * muzzlesize) + (ddir * muzzlesize), side.xend,   side.ystart, kocol);
		va->AddVertexQTC(pos1 - (dir1 * muzzlesize) + (ddir * muzzlesize), side.xend,   side.yend,   kocol);
		va->AddVertexQTC(pos1 - (dir1 * muzzlesize),                       side.xstart, side.yend,   kocol);
		muzzlesize = muzzlesize * 0.6f;
		va->AddVertexQTC(pos1 + (dir1 * muzzlesize),                       side.xstart, side.ystart, corcol);
		va->AddVertexQTC(pos1 + (dir1 * muzzlesize) + (ddir * muzzlesize), side.xend,   side.ystart, corcol);
		va->AddVertexQTC(pos1 - (dir1 * muzzlesize) + (ddir * muzzlesize), side.xend,   side.yend,   corcol);
		va->AddVertexQTC(pos1 - (dir1 * muzzlesize),                       side.xstart, side.yend,   corcol);

		starttex += 0.5f;
		if (starttex > 1) {
			starttex = starttex - 1;
		}
		for (int i = 0; i < 3; i++) {
			corcol[i] = (int)(corecolstart[i] * (1 - starttex));
			kocol[i]  = (int)(kocolstart[i]   * (1 - starttex));
		}
		muzzlesize = size * flaresize * starttex;
		va->AddVertexQTC(pos1 + (dir1 * muzzlesize),                       side.xstart, side.ystart, kocol);
		va->AddVertexQTC(pos1 + (dir1 * muzzlesize) + (ddir * muzzlesize), side.xend,   side.ystart, kocol);
		va->AddVertexQTC(pos1 - (dir1 * muzzlesize) + (ddir * muzzlesize), side.xend,   side.yend,   kocol);
		va->AddVertexQTC(pos1 - (dir1 * muzzlesize),                       side.xstart, side.yend,   kocol);
		muzzlesize = muzzlesize * 0.6f;
		va->AddVertexQTC(pos1 + (dir1 * muzzlesize),                       side.xstart, side.ystart, corcol);
		va->AddVertexQTC(pos1 + (dir1 * muzzlesize) + (ddir * muzzlesize), side.xend,   side.ystart, corcol);
		va->AddVertexQTC(pos1 - (dir1 * muzzlesize) + (ddir * muzzlesize), side.xend,   side.yend,   corcol);
		va->AddVertexQTC(pos1 - (dir1 * muzzlesize),                       side.xstart, side.yend,   corcol);

	// draw flare
	float fsize = size * flaresize;
	// move flare slightly in camera direction
	pos1 = startPos - (camera->forward * 3);
	va->AddVertexQTC(pos1 - (camera->right * fsize-camera->up * fsize), weaponDef->visuals.texture4->xstart, weaponDef->visuals.texture4->ystart, kocolstart);
	va->AddVertexQTC(pos1 + (camera->right * fsize-camera->up * fsize), weaponDef->visuals.texture4->xend,   weaponDef->visuals.texture4->ystart, kocolstart);
	va->AddVertexQTC(pos1 + (camera->right * fsize+camera->up * fsize), weaponDef->visuals.texture4->xend,   weaponDef->visuals.texture4->yend,   kocolstart);
	va->AddVertexQTC(pos1 - (camera->right * fsize+camera->up * fsize), weaponDef->visuals.texture4->xstart, weaponDef->visuals.texture4->yend,   kocolstart);

	fsize = fsize*corethickness;
	va->AddVertexQTC(pos1 - (camera->right * fsize-camera->up * fsize), weaponDef->visuals.texture4->xstart, weaponDef->visuals.texture4->ystart, corecolstart);
	va->AddVertexQTC(pos1 + (camera->right * fsize-camera->up * fsize), weaponDef->visuals.texture4->xend,   weaponDef->visuals.texture4->ystart, corecolstart);
	va->AddVertexQTC(pos1 + (camera->right * fsize+camera->up * fsize), weaponDef->visuals.texture4->xend,   weaponDef->visuals.texture4->yend,   corecolstart);
	va->AddVertexQTC(pos1 - (camera->right * fsize+camera->up * fsize), weaponDef->visuals.texture4->xstart, weaponDef->visuals.texture4->yend,   corecolstart);

}

void CLargeBeamLaserProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	unsigned char color[4] = { kocolstart[0], kocolstart[1], kocolstart[2], 255 };
	lines.AddVertexQC(startPos, color);
	lines.AddVertexQC(endPos,   color);
}
