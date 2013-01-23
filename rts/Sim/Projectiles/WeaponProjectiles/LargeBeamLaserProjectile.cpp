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
	CR_MEMBER(coreColStart),
	CR_MEMBER(edgeColStart),
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

	coreColStart[0] = (color2.x * 255);
	coreColStart[1] = (color2.y * 255);
	coreColStart[2] = (color2.z * 255);
	coreColStart[3] = 1;
	edgeColStart[0] = (color.x * 255);
	edgeColStart[1] = (color.y * 255);
	edgeColStart[2] = (color.z * 255);
	edgeColStart[3] = 1;

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
			coreColStart[i] = (unsigned char) (coreColStart[i] * decay);
			edgeColStart[i] = (unsigned char) (edgeColStart[i] * decay);
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

	const float3 cameraDir = (pos - camera->pos).SafeANormalize();
	// beam's coor-system; degenerate if targetPos == startPos
	const float3 zdir = (targetPos - startpos).SafeANormalize();
	const float3 xdir = (cameraDir.cross(zdir)).SafeANormalize();
	const float3 ydir = (cameraDir.cross(xdir));

	float3 pos1 = startpos;
	float3 pos2 = targetPos;

	const float startTex = 1.0f - ((gu->modGameTime * scrollspeed) - int(gu->modGameTime * scrollspeed));
	const float texSizeX = beamtex.xend - beamtex.xstart;

	const float beamEdgeSize = thickness;
	const float beamCoreSize = beamEdgeSize * corethickness;
	const float flareEdgeSize = thickness * flaresize;
	const float flareCoreSize = flareEdgeSize * corethickness;

	const float beamLength = (targetPos - startpos).dot(zdir);
	const float polyLength = texSizeX * (1.0f / texSizeX) * tilelength; // ??

	const float istart = polyLength * (1.0f - startTex);
	const float iend = beamLength - tilelength;

	AtlasedTexture tex = beamtex;

	va->EnlargeArrays(64 + (8 * (int((iend - istart) / tilelength) + 2)), 0, VA_SIZE_TC);

	#define WT2 weaponDef->visuals.texture2
	#define WT4 weaponDef->visuals.texture4

	if (istart > beamLength) {
		// beam short enough to be drawn by one polygon
		// draw laser start
		tex.xstart = beamtex.xstart + startTex * texSizeX;

		va->AddVertexQTC(pos1 - (ydir * beamEdgeSize), tex.xstart, tex.ystart, edgeColStart);
		va->AddVertexQTC(pos1 + (ydir * beamEdgeSize), tex.xstart, tex.yend,   edgeColStart);
		va->AddVertexQTC(pos2 + (ydir * beamEdgeSize), tex.xend,   tex.yend,   edgeColStart);
		va->AddVertexQTC(pos2 - (ydir * beamEdgeSize), tex.xend,   tex.ystart, edgeColStart);
		va->AddVertexQTC(pos1 - (ydir * beamCoreSize), tex.xstart, tex.ystart, coreColStart);
		va->AddVertexQTC(pos1 + (ydir * beamCoreSize), tex.xstart, tex.yend,   coreColStart);
		va->AddVertexQTC(pos2 + (ydir * beamCoreSize), tex.xend,   tex.yend,   coreColStart);
		va->AddVertexQTC(pos2 - (ydir * beamCoreSize), tex.xend,   tex.ystart, coreColStart);
	} else {
		// beam longer than one polygon
		pos2 = pos1 + zdir * istart;

		// draw laser start
		tex.xstart = beamtex.xstart + startTex * texSizeX;

		va->AddVertexQTC(pos1 - (ydir * beamEdgeSize), tex.xstart, tex.ystart, edgeColStart);
		va->AddVertexQTC(pos1 + (ydir * beamEdgeSize), tex.xstart, tex.yend,   edgeColStart);
		va->AddVertexQTC(pos2 + (ydir * beamEdgeSize), tex.xend,   tex.yend,   edgeColStart);
		va->AddVertexQTC(pos2 - (ydir * beamEdgeSize), tex.xend,   tex.ystart, edgeColStart);
		va->AddVertexQTC(pos1 - (ydir * beamCoreSize), tex.xstart, tex.ystart, coreColStart);
		va->AddVertexQTC(pos1 + (ydir * beamCoreSize), tex.xstart, tex.yend,   coreColStart);
		va->AddVertexQTC(pos2 + (ydir * beamCoreSize), tex.xend,   tex.yend,   coreColStart);
		va->AddVertexQTC(pos2 - (ydir * beamCoreSize), tex.xend,   tex.ystart, coreColStart);

		// draw continous beam
		tex.xstart = beamtex.xstart;

		for (float i = istart; i < iend; i += tilelength) {
			//! CAUTION: loop count must match EnlargeArrays above
			pos1 = startpos + zdir * i;
			pos2 = startpos + zdir * (i + tilelength);

			va->AddVertexQTC(pos1 - (ydir * beamEdgeSize), tex.xstart, tex.ystart, edgeColStart);
			va->AddVertexQTC(pos1 + (ydir * beamEdgeSize), tex.xstart, tex.yend,   edgeColStart);
			va->AddVertexQTC(pos2 + (ydir * beamEdgeSize), tex.xend,   tex.yend,   edgeColStart);
			va->AddVertexQTC(pos2 - (ydir * beamEdgeSize), tex.xend,   tex.ystart, edgeColStart);
			va->AddVertexQTC(pos1 - (ydir * beamCoreSize), tex.xstart, tex.ystart, coreColStart);
			va->AddVertexQTC(pos1 + (ydir * beamCoreSize), tex.xstart, tex.yend,   coreColStart);
			va->AddVertexQTC(pos2 + (ydir * beamCoreSize), tex.xend,   tex.yend,   coreColStart);
			va->AddVertexQTC(pos2 - (ydir * beamCoreSize), tex.xend,   tex.ystart, coreColStart);
		}

		// draw laser end
		pos1 = startpos + zdir * iend;
		pos2 = targetPos;
		tex.xend = tex.xstart + (pos1.distance(pos2) / tilelength) * texSizeX;

		va->AddVertexQTC(pos1 - (ydir * beamEdgeSize), tex.xstart, tex.ystart, edgeColStart);
		va->AddVertexQTC(pos1 + (ydir * beamEdgeSize), tex.xstart, tex.yend,   edgeColStart);
		va->AddVertexQTC(pos2 + (ydir * beamEdgeSize), tex.xend,   tex.yend,   edgeColStart);
		va->AddVertexQTC(pos2 - (ydir * beamEdgeSize), tex.xend,   tex.ystart, edgeColStart);
		va->AddVertexQTC(pos1 - (ydir * beamCoreSize), tex.xstart, tex.ystart, coreColStart);
		va->AddVertexQTC(pos1 + (ydir * beamCoreSize), tex.xstart, tex.yend,   coreColStart);
		va->AddVertexQTC(pos2 + (ydir * beamCoreSize), tex.xend,   tex.yend,   coreColStart);
		va->AddVertexQTC(pos2 - (ydir * beamCoreSize), tex.xend,   tex.ystart, coreColStart);
	}

	va->AddVertexQTC(pos2 - (ydir * beamEdgeSize),                         WT2->xstart, WT2->ystart, edgeColStart);
	va->AddVertexQTC(pos2 + (ydir * beamEdgeSize),                         WT2->xstart, WT2->yend,   edgeColStart);
	va->AddVertexQTC(pos2 + (ydir * beamEdgeSize) + (xdir * beamEdgeSize), WT2->xend,   WT2->yend,   edgeColStart);
	va->AddVertexQTC(pos2 - (ydir * beamEdgeSize) + (xdir * beamEdgeSize), WT2->xend,   WT2->ystart, edgeColStart);
	va->AddVertexQTC(pos2 - (ydir * beamCoreSize),                         WT2->xstart, WT2->ystart, coreColStart);
	va->AddVertexQTC(pos2 + (ydir * beamCoreSize),                         WT2->xstart, WT2->yend,   coreColStart);
	va->AddVertexQTC(pos2 + (ydir * beamCoreSize) + (xdir * beamCoreSize), WT2->xend,   WT2->yend,   coreColStart);
	va->AddVertexQTC(pos2 - (ydir * beamCoreSize) + (xdir * beamCoreSize), WT2->xend,   WT2->ystart, coreColStart);

	float pulseStartTime = (gu->modGameTime * pulseSpeed) - int(gu->modGameTime * pulseSpeed);
	float muzzleEdgeSize = thickness * flaresize * pulseStartTime;
	float muzzleCoreSize = muzzleEdgeSize * 0.6f;

	unsigned char coreColor[4] = {0, 0, 0, 1};
	unsigned char edgeColor[4] = {0, 0, 0, 1};

	for (int i = 0; i < 3; i++) {
		coreColor[i] = int(coreColStart[i] * (1.0f - pulseStartTime));
		edgeColor[i] = int(edgeColStart[i] * (1.0f - pulseStartTime));
	}

	{
		// draw muzzleflare
		pos1 = startpos - zdir * (thickness * flaresize) * 0.02f;

		va->AddVertexQTC(pos1 + (ydir * muzzleEdgeSize),                           side.xstart, side.ystart, edgeColor);
		va->AddVertexQTC(pos1 + (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), side.xend,   side.ystart, edgeColor);
		va->AddVertexQTC(pos1 - (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), side.xend,   side.yend,   edgeColor);
		va->AddVertexQTC(pos1 - (ydir * muzzleEdgeSize),                           side.xstart, side.yend,   edgeColor);

		va->AddVertexQTC(pos1 + (ydir * muzzleCoreSize),                           side.xstart, side.ystart, coreColor);
		va->AddVertexQTC(pos1 + (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), side.xend,   side.ystart, coreColor);
		va->AddVertexQTC(pos1 - (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), side.xend,   side.yend,   coreColor);
		va->AddVertexQTC(pos1 - (ydir * muzzleCoreSize),                           side.xstart, side.yend,   coreColor);

		pulseStartTime += 0.5f;
		pulseStartTime -= (1.0f * (pulseStartTime > 1.0f));

		for (int i = 0; i < 3; i++) {
			coreColor[i] = int(coreColStart[i] * (1.0f - pulseStartTime));
			edgeColor[i] = int(edgeColStart[i] * (1.0f - pulseStartTime));
		}

		muzzleEdgeSize = thickness * flaresize * pulseStartTime;

		va->AddVertexQTC(pos1 + (ydir * muzzleEdgeSize),                           side.xstart, side.ystart, edgeColor);
		va->AddVertexQTC(pos1 + (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), side.xend,   side.ystart, edgeColor);
		va->AddVertexQTC(pos1 - (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), side.xend,   side.yend,   edgeColor);
		va->AddVertexQTC(pos1 - (ydir * muzzleEdgeSize),                           side.xstart, side.yend,   edgeColor);

		muzzleCoreSize = muzzleEdgeSize * 0.6f;

		va->AddVertexQTC(pos1 + (ydir * muzzleCoreSize),                           side.xstart, side.ystart, coreColor);
		va->AddVertexQTC(pos1 + (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), side.xend,   side.ystart, coreColor);
		va->AddVertexQTC(pos1 - (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), side.xend,   side.yend,   coreColor);
		va->AddVertexQTC(pos1 - (ydir * muzzleCoreSize),                           side.xstart, side.yend,   coreColor);
	}

	{
		// draw flare (moved slightly along the camera direction)
		pos1 = startpos - (camera->forward * 3.0f);

		va->AddVertexQTC(pos1 - (camera->right * flareEdgeSize) - (camera->up * flareEdgeSize), WT4->xstart, WT4->ystart, edgeColStart);
		va->AddVertexQTC(pos1 + (camera->right * flareEdgeSize) - (camera->up * flareEdgeSize), WT4->xend,   WT4->ystart, edgeColStart);
		va->AddVertexQTC(pos1 + (camera->right * flareEdgeSize) + (camera->up * flareEdgeSize), WT4->xend,   WT4->yend,   edgeColStart);
		va->AddVertexQTC(pos1 - (camera->right * flareEdgeSize) + (camera->up * flareEdgeSize), WT4->xstart, WT4->yend,   edgeColStart);

		va->AddVertexQTC(pos1 - (camera->right * flareCoreSize) - (camera->up * flareCoreSize), WT4->xstart, WT4->ystart, coreColStart);
		va->AddVertexQTC(pos1 + (camera->right * flareCoreSize) - (camera->up * flareCoreSize), WT4->xend,   WT4->ystart, coreColStart);
		va->AddVertexQTC(pos1 + (camera->right * flareCoreSize) + (camera->up * flareCoreSize), WT4->xend,   WT4->yend,   coreColStart);
		va->AddVertexQTC(pos1 - (camera->right * flareCoreSize) + (camera->up * flareCoreSize), WT4->xstart, WT4->yend,   coreColStart);
	}

	#undef WT4
	#undef WT2
}

void CLargeBeamLaserProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	const unsigned char color[4] = {edgeColStart[0], edgeColStart[1], edgeColStart[2], 255};

	lines.AddVertexQC(startpos,  color);
	lines.AddVertexQC(targetPos, color);
}
