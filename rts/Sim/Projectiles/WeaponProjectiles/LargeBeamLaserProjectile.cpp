/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LargeBeamLaserProjectile.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/myMath.h"

CR_BIND_DERIVED(CLargeBeamLaserProjectile, CWeaponProjectile, (ProjectileParams(), ZeroVector, ZeroVector));

CR_REG_METADATA(CLargeBeamLaserProjectile,(
	CR_SETFLAG(CF_Synced),
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

CLargeBeamLaserProjectile::CLargeBeamLaserProjectile(const ProjectileParams& params, const float3& color, const float3& color2)
	: CWeaponProjectile(params, true)
	, decay(1.0f)
{
	projectileType = WEAPON_LARGEBEAMLASER_PROJECTILE;
	checkCol = false;
	useAirLos = true;

	if (weaponDef) {
		this->beamtex = *weaponDef->visuals.texture1;
		this->side    = *weaponDef->visuals.texture3;
	}

	SetRadiusAndHeight(pos.distance(targetPos), 0.0f);

	corecolstart[0] = (color2.x * 255);
	corecolstart[1] = (color2.y * 255);
	corecolstart[2] = (color2.z * 255);
	corecolstart[3] = 1;
	kocolstart[0]   = (color.x * 255);
	kocolstart[1]   = (color.y * 255);
	kocolstart[2]   = (color.z * 255);
	kocolstart[3]   = 1;

	if (weaponDef) {
		thickness     = weaponDef->visuals.thickness;
		corethickness = weaponDef->visuals.corethickness;
		flaresize     = weaponDef->visuals.laserflaresize;
		tilelength    = weaponDef->visuals.tilelength;
		scrollspeed   = weaponDef->visuals.scrollspeed;
		pulseSpeed    = weaponDef->visuals.pulseSpeed;
		decay         = weaponDef->visuals.beamdecay;
	}

	cegID = gCEG->Load(explGenHandler, (weaponDef != NULL)? weaponDef->cegTag: "");
}



void CLargeBeamLaserProjectile::Update()
{
	if (ttl > 0) {
		for (int i = 0; i < 3; i++) {
			corecolstart[i] = (unsigned char) (corecolstart[i] * decay);
			kocolstart[i] = (unsigned char) (kocolstart[i] * decay);
		}

		gCEG->Explosion(cegID, startpos + ((targetPos - startpos) / ttl), 0.0f, flaresize, NULL, 0.0f, NULL, targetPos - startpos);
		ttl--;
	}
	else {
		deleteMe = true;
	}

	UpdateInterception();
}

void CLargeBeamLaserProjectile::Draw()
{
	inArray = true;

	float3 dif(pos - camera->pos);
	float3 ddir(targetPos - startpos);
	const float camDist = dif.Length();
	const float beamlength = ddir.Length();
	dif /= camDist;
	ddir = ddir / beamlength;

	const float3 dir1((dif.cross(ddir)).Normalize());
	const float3 dir2(dif.cross(dir1));

	float3 pos1 = startpos;
	float3 pos2 = targetPos;

	float starttex = (gu->modGameTime) * scrollspeed;
	starttex = 1.0f - (starttex - (int)starttex);

	const float texxsize = beamtex.xend - beamtex.xstart;
	const float& size = thickness;
	const float& coresize = size * corethickness;

	const float polylength = (beamtex.xend - beamtex.xstart) * (1.0f / texxsize) * tilelength;

	const float istart = polylength * (1.0f - starttex);
	const float iend = beamlength - tilelength;

	AtlasedTexture tex = beamtex;

	va->EnlargeArrays(64 + (8 * ((int)((iend - istart) / tilelength) + 2)), 0, VA_SIZE_TC);
	if (istart > beamlength) {
		// beam short enough to be drawn by one polygon
		// draw laser start
		tex.xstart = beamtex.xstart + starttex * ((beamtex.xend - beamtex.xstart));

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
		for (i = istart; i < iend; i += tilelength) {
			//! CAUTION: loop count must match EnlargeArrays above
			pos1 = startpos + ddir * i;
			pos2 = startpos + ddir * (i + tilelength);

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
		pos1 = startpos + ddir * i;
		pos2 = targetPos;
		tex.xend = tex.xstart + (pos1.distance(pos2) / tilelength) * texxsize;

		va->AddVertexQTC(pos1 - (dir1 * size),     tex.xstart, tex.ystart, kocolstart);
		va->AddVertexQTC(pos1 + (dir1 * size),     tex.xstart, tex.yend,   kocolstart);
		va->AddVertexQTC(pos2 + (dir1 * size),     tex.xend,   tex.yend,   kocolstart);
		va->AddVertexQTC(pos2 - (dir1 * size),     tex.xend,   tex.ystart, kocolstart);
		va->AddVertexQTC(pos1 - (dir1 * coresize), tex.xstart, tex.ystart, corecolstart);
		va->AddVertexQTC(pos1 + (dir1 * coresize), tex.xstart, tex.yend,   corecolstart);
		va->AddVertexQTC(pos2 + (dir1 * coresize), tex.xend,   tex.yend,   corecolstart);
		va->AddVertexQTC(pos2 - (dir1 * coresize), tex.xend,   tex.ystart, corecolstart);
	}

	#define WT2 weaponDef->visuals.texture2
	va->AddVertexQTC(pos2 - (dir1 * size),                         WT2->xstart, WT2->ystart, kocolstart);
	va->AddVertexQTC(pos2 + (dir1 * size),                         WT2->xstart, WT2->yend,   kocolstart);
	va->AddVertexQTC(pos2 + (dir1 * size) + (dir2 * size),         WT2->xend,   WT2->yend,   kocolstart);
	va->AddVertexQTC(pos2 - (dir1 * size) + (dir2 * size),         WT2->xend,   WT2->ystart, kocolstart);
	va->AddVertexQTC(pos2 - (dir1 * coresize),                     WT2->xstart, WT2->ystart, corecolstart);
	va->AddVertexQTC(pos2 + (dir1 * coresize),                     WT2->xstart, WT2->yend,   corecolstart);
	va->AddVertexQTC(pos2 + (dir1 * coresize) + (dir2 * coresize), WT2->xend,   WT2->yend,   corecolstart);
	va->AddVertexQTC(pos2 - (dir1 * coresize) + (dir2 * coresize), WT2->xend,   WT2->ystart, corecolstart);
	#undef WT2

	starttex  = gu->modGameTime * pulseSpeed;
	starttex -= (int)starttex;

	float muzzlesize = size * flaresize * starttex;
	float fsize = size * flaresize;

	unsigned char corcol[4] = {0, 0, 0, 1};
	unsigned char kocol[4] = {0, 0, 0, 1};

	for (int i = 0; i < 3; i++) {
		corcol[i] = (int)(corecolstart[i] * (1 - starttex));
		kocol[i]  = (int)(kocolstart[i]   * (1 - starttex));
	}

	{
		// draw muzzleflare
		pos1 = startpos - ddir * (size * flaresize) * 0.02f;

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
	}

	{
		// draw flare (moved slightly along the camera direction)
		pos1 = startpos - (camera->forward * 3.0f);

		#define WT4 weaponDef->visuals.texture4
		va->AddVertexQTC(pos1 - (camera->right * fsize) - (camera->up * fsize), WT4->xstart, WT4->ystart, kocolstart);
		va->AddVertexQTC(pos1 + (camera->right * fsize) - (camera->up * fsize), WT4->xend,   WT4->ystart, kocolstart);
		va->AddVertexQTC(pos1 + (camera->right * fsize) + (camera->up * fsize), WT4->xend,   WT4->yend,   kocolstart);
		va->AddVertexQTC(pos1 - (camera->right * fsize) + (camera->up * fsize), WT4->xstart, WT4->yend,   kocolstart);

		fsize = fsize * corethickness;
		va->AddVertexQTC(pos1 - (camera->right * fsize) - (camera->up * fsize), WT4->xstart, WT4->ystart, corecolstart);
		va->AddVertexQTC(pos1 + (camera->right * fsize) - (camera->up * fsize), WT4->xend,   WT4->ystart, corecolstart);
		va->AddVertexQTC(pos1 + (camera->right * fsize) + (camera->up * fsize), WT4->xend,   WT4->yend,   corecolstart);
		va->AddVertexQTC(pos1 - (camera->right * fsize) + (camera->up * fsize), WT4->xstart, WT4->yend,   corecolstart);
		#undef WT4
	}
}

void CLargeBeamLaserProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	unsigned char color[4] = { kocolstart[0], kocolstart[1], kocolstart[2], 255 };
	lines.AddVertexQC(startpos,  color);
	lines.AddVertexQC(targetPos, color);
}
