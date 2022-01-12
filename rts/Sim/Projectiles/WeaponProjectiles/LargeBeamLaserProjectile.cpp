/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LargeBeamLaserProjectile.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/SpringMath.h"
#include <cstring> //memset

CR_BIND_DERIVED(CLargeBeamLaserProjectile, CWeaponProjectile, )

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
	CR_MEMBER(decay),
	CR_MEMBER(beamtex),
	CR_MEMBER(sidetex)
))


CLargeBeamLaserProjectile::CLargeBeamLaserProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	, thickness(0.0f)
	, corethickness(0.0f)
	, flaresize(0.0f)
	, tilelength(0.0f)
	, scrollspeed(0.0f)
	, pulseSpeed(0.0f)
	, decay(1.0f)
{
	projectileType = WEAPON_LARGEBEAMLASER_PROJECTILE;

	if (weaponDef != NULL) {
		assert(weaponDef->IsHitScanWeapon());

		thickness     = weaponDef->visuals.thickness;
		corethickness = weaponDef->visuals.corethickness;
		flaresize     = weaponDef->visuals.laserflaresize;
		tilelength    = weaponDef->visuals.tilelength;
		scrollspeed   = weaponDef->visuals.scrollspeed;
		pulseSpeed    = weaponDef->visuals.pulseSpeed;
		decay         = weaponDef->visuals.beamdecay;

		beamtex       = *weaponDef->visuals.texture1;
		sidetex       = *weaponDef->visuals.texture3;

		coreColStart[0] = (weaponDef->visuals.color2.x * 255);
		coreColStart[1] = (weaponDef->visuals.color2.y * 255);
		coreColStart[2] = (weaponDef->visuals.color2.z * 255);
		coreColStart[3] = 1;
		edgeColStart[0] = (weaponDef->visuals.color.x * 255);
		edgeColStart[1] = (weaponDef->visuals.color.y * 255);
		edgeColStart[2] = (weaponDef->visuals.color.z * 255);
		edgeColStart[3] = 1;
	} else {
		memset(&coreColStart[0], 0, sizeof(coreColStart));
		memset(&edgeColStart[0], 0, sizeof(edgeColStart));
	}
}



void CLargeBeamLaserProjectile::Update()
{
	if ((--ttl) <= 0) {
		deleteMe = true;
	} else {
		for (int i = 0; i < 3; i++) {
			coreColStart[i] = (unsigned char) (coreColStart[i] * decay);
			edgeColStart[i] = (unsigned char) (edgeColStart[i] * decay);
		}

		explGenHandler.GenExplosion(cegID, startPos + ((targetPos - startPos) / ttl), (targetPos - startPos), 0.0f, flaresize, 0.0f, NULL, NULL);
	}

	UpdateInterception();
}

void CLargeBeamLaserProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	const float3 midPos = (targetPos + startPos) * 0.5f;
	const float3 cameraDir = (midPos - camera->GetPos()).SafeANormalize();
	// beam's coor-system; degenerate if targetPos == startPos
	const float3 zdir = (targetPos - startPos).SafeANormalize();
	const float3 xdir = (cameraDir.cross(zdir)).SafeANormalize();
	const float3 ydir = (cameraDir.cross(xdir));

	const float3& camR = camera->GetRight();
	const float3& camU = camera->GetUp();

	float3 pos1 = startPos;
	float3 pos2 = targetPos;

	const float startTex = 1.0f - ((gu->modGameTime * scrollspeed) - int(gu->modGameTime * scrollspeed));
	const float texSizeX = beamtex.xend - beamtex.xstart;

	const float beamEdgeSize = thickness;
	const float beamCoreSize = beamEdgeSize * corethickness;
	const float beamLength   = (targetPos - startPos).dot(zdir);
	const float flareEdgeSize = thickness * flaresize;
	const float flareCoreSize = flareEdgeSize * corethickness;

	const float beamTileMinDst = tilelength * (1.0f - startTex);
	const float beamTileMaxDst = beamLength - tilelength;
	// note: beamTileMaxDst can be negative, in which case we want numBeamTiles to equal zero
	const float numBeamTiles = std::floor(((std::max(beamTileMinDst, beamTileMaxDst) - beamTileMinDst) / tilelength) + 0.5f);

	AtlasedTexture tex = beamtex;

	#define WT2 weaponDef->visuals.texture2
	#define WT4 weaponDef->visuals.texture4

	if (beamTileMinDst > beamLength) {
		// beam short enough to be drawn by one polygon
		// draw laser start
		tex.xstart = beamtex.xstart + startTex * texSizeX;

		{
			va->SafeAppend({pos1 - (xdir * beamEdgeSize), tex.xstart, tex.ystart, edgeColStart});
			va->SafeAppend({pos1 + (xdir * beamEdgeSize), tex.xstart, tex.yend,   edgeColStart});
			va->SafeAppend({pos2 + (xdir * beamEdgeSize), tex.xend,   tex.yend,   edgeColStart});

			va->SafeAppend({pos2 + (xdir * beamEdgeSize), tex.xend,   tex.yend,   edgeColStart});
			va->SafeAppend({pos2 - (xdir * beamEdgeSize), tex.xend,   tex.ystart, edgeColStart});
			va->SafeAppend({pos1 - (xdir * beamEdgeSize), tex.xstart, tex.ystart, edgeColStart});
		}
		{
			va->SafeAppend({pos1 - (xdir * beamCoreSize), tex.xstart, tex.ystart, coreColStart});
			va->SafeAppend({pos1 + (xdir * beamCoreSize), tex.xstart, tex.yend,   coreColStart});
			va->SafeAppend({pos2 + (xdir * beamCoreSize), tex.xend,   tex.yend,   coreColStart});

			va->SafeAppend({pos2 + (xdir * beamCoreSize), tex.xend,   tex.yend,   coreColStart});
			va->SafeAppend({pos2 - (xdir * beamCoreSize), tex.xend,   tex.ystart, coreColStart});
			va->SafeAppend({pos1 - (xdir * beamCoreSize), tex.xstart, tex.ystart, coreColStart});
		}
	} else {
		// beam longer than one polygon
		pos2 = pos1 + zdir * beamTileMinDst;

		// draw laser start
		tex.xstart = beamtex.xstart + startTex * texSizeX;

		{
			va->SafeAppend({pos1 - (xdir * beamEdgeSize), tex.xstart, tex.ystart, edgeColStart});
			va->SafeAppend({pos1 + (xdir * beamEdgeSize), tex.xstart, tex.yend,   edgeColStart});
			va->SafeAppend({pos2 + (xdir * beamEdgeSize), tex.xend,   tex.yend,   edgeColStart});

			va->SafeAppend({pos2 + (xdir * beamEdgeSize), tex.xend,   tex.yend,   edgeColStart});
			va->SafeAppend({pos2 - (xdir * beamEdgeSize), tex.xend,   tex.ystart, edgeColStart});
			va->SafeAppend({pos1 - (xdir * beamEdgeSize), tex.xstart, tex.ystart, edgeColStart});
		}
		{
			va->SafeAppend({pos1 - (xdir * beamCoreSize), tex.xstart, tex.ystart, coreColStart});
			va->SafeAppend({pos1 + (xdir * beamCoreSize), tex.xstart, tex.yend,   coreColStart});
			va->SafeAppend({pos2 + (xdir * beamCoreSize), tex.xend,   tex.yend,   coreColStart});

			va->SafeAppend({pos2 + (xdir * beamCoreSize), tex.xend,   tex.yend,   coreColStart});
			va->SafeAppend({pos2 - (xdir * beamCoreSize), tex.xend,   tex.ystart, coreColStart});
			va->SafeAppend({pos1 - (xdir * beamCoreSize), tex.xstart, tex.ystart, coreColStart});
		}

		// draw continous beam
		tex.xstart = beamtex.xstart;

		for (float i = beamTileMinDst; i < beamTileMaxDst; i += tilelength) {
			pos1 = startPos + zdir * i;
			pos2 = startPos + zdir * (i + tilelength);

			{
				va->SafeAppend({pos1 - (xdir * beamEdgeSize), tex.xstart, tex.ystart, edgeColStart});
				va->SafeAppend({pos1 + (xdir * beamEdgeSize), tex.xstart, tex.yend,   edgeColStart});
				va->SafeAppend({pos2 + (xdir * beamEdgeSize), tex.xend,   tex.yend,   edgeColStart});

				va->SafeAppend({pos2 + (xdir * beamEdgeSize), tex.xend,   tex.yend,   edgeColStart});
				va->SafeAppend({pos2 - (xdir * beamEdgeSize), tex.xend,   tex.ystart, edgeColStart});
				va->SafeAppend({pos1 - (xdir * beamEdgeSize), tex.xstart, tex.ystart, edgeColStart});
			}
			{
				va->SafeAppend({pos1 - (xdir * beamCoreSize), tex.xstart, tex.ystart, coreColStart});
				va->SafeAppend({pos1 + (xdir * beamCoreSize), tex.xstart, tex.yend,   coreColStart});
				va->SafeAppend({pos2 + (xdir * beamCoreSize), tex.xend,   tex.yend,   coreColStart});

				va->SafeAppend({pos2 + (xdir * beamCoreSize), tex.xend,   tex.yend,   coreColStart});
				va->SafeAppend({pos2 - (xdir * beamCoreSize), tex.xend,   tex.ystart, coreColStart});
				va->SafeAppend({pos1 - (xdir * beamCoreSize), tex.xstart, tex.ystart, coreColStart});
			}
		}

		// draw laser end
		pos1 = startPos + zdir * (beamTileMinDst + numBeamTiles * tilelength);
		pos2 = targetPos;
		tex.xend = tex.xstart + (pos1.distance(pos2) / tilelength) * texSizeX;

		{
			va->SafeAppend({pos1 - (xdir * beamEdgeSize), tex.xstart, tex.ystart, edgeColStart});
			va->SafeAppend({pos1 + (xdir * beamEdgeSize), tex.xstart, tex.yend,   edgeColStart});
			va->SafeAppend({pos2 + (xdir * beamEdgeSize), tex.xend,   tex.yend,   edgeColStart});

			va->SafeAppend({pos2 + (xdir * beamEdgeSize), tex.xend,   tex.yend,   edgeColStart});
			va->SafeAppend({pos2 - (xdir * beamEdgeSize), tex.xend,   tex.ystart, edgeColStart});
			va->SafeAppend({pos1 - (xdir * beamEdgeSize), tex.xstart, tex.ystart, edgeColStart});
		}
		{
			va->SafeAppend({pos1 - (xdir * beamCoreSize), tex.xstart, tex.ystart, coreColStart});
			va->SafeAppend({pos1 + (xdir * beamCoreSize), tex.xstart, tex.yend,   coreColStart});
			va->SafeAppend({pos2 + (xdir * beamCoreSize), tex.xend,   tex.yend,   coreColStart});

			va->SafeAppend({pos2 + (xdir * beamCoreSize), tex.xend,   tex.yend,   coreColStart});
			va->SafeAppend({pos2 - (xdir * beamCoreSize), tex.xend,   tex.ystart, coreColStart});
			va->SafeAppend({pos1 - (xdir * beamCoreSize), tex.xstart, tex.ystart, coreColStart});
		}
	}

	{
		va->SafeAppend({pos2 - (xdir * beamEdgeSize),                         WT2->xstart, WT2->ystart, edgeColStart});
		va->SafeAppend({pos2 + (xdir * beamEdgeSize),                         WT2->xstart, WT2->yend,   edgeColStart});
		va->SafeAppend({pos2 + (xdir * beamEdgeSize) + (ydir * beamEdgeSize), WT2->xend,   WT2->yend,   edgeColStart});

		va->SafeAppend({pos2 + (xdir * beamEdgeSize) + (ydir * beamEdgeSize), WT2->xend,   WT2->yend,   edgeColStart});
		va->SafeAppend({pos2 - (xdir * beamEdgeSize) + (ydir * beamEdgeSize), WT2->xend,   WT2->ystart, edgeColStart});
		va->SafeAppend({pos2 - (xdir * beamEdgeSize),                         WT2->xstart, WT2->ystart, edgeColStart});
	}
	{
		va->SafeAppend({pos2 - (xdir * beamCoreSize),                         WT2->xstart, WT2->ystart, coreColStart});
		va->SafeAppend({pos2 + (xdir * beamCoreSize),                         WT2->xstart, WT2->yend,   coreColStart});
		va->SafeAppend({pos2 + (xdir * beamCoreSize) + (ydir * beamCoreSize), WT2->xend,   WT2->yend,   coreColStart});

		va->SafeAppend({pos2 + (xdir * beamCoreSize) + (ydir * beamCoreSize), WT2->xend,   WT2->yend,   coreColStart});
		va->SafeAppend({pos2 - (xdir * beamCoreSize) + (ydir * beamCoreSize), WT2->xend,   WT2->ystart, coreColStart});
		va->SafeAppend({pos2 - (xdir * beamCoreSize),                         WT2->xstart, WT2->ystart, coreColStart});
	}

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
		pos1 = startPos - zdir * (thickness * flaresize) * 0.02f;

		{
			va->SafeAppend({pos1 + (ydir * muzzleEdgeSize),                           sidetex.xstart, sidetex.ystart, edgeColor});
			va->SafeAppend({pos1 + (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), sidetex.xend,   sidetex.ystart, edgeColor});
			va->SafeAppend({pos1 - (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), sidetex.xend,   sidetex.yend,   edgeColor});

			va->SafeAppend({pos1 - (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), sidetex.xend,   sidetex.yend,   edgeColor});
			va->SafeAppend({pos1 - (ydir * muzzleEdgeSize),                           sidetex.xstart, sidetex.yend,   edgeColor});
			va->SafeAppend({pos1 + (ydir * muzzleEdgeSize),                           sidetex.xstart, sidetex.ystart, edgeColor});
		}
		{
			va->SafeAppend({pos1 + (ydir * muzzleCoreSize),                           sidetex.xstart, sidetex.ystart, coreColor});
			va->SafeAppend({pos1 + (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), sidetex.xend,   sidetex.ystart, coreColor});
			va->SafeAppend({pos1 - (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), sidetex.xend,   sidetex.yend,   coreColor});

			va->SafeAppend({pos1 - (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), sidetex.xend,   sidetex.yend,   coreColor});
			va->SafeAppend({pos1 - (ydir * muzzleCoreSize),                           sidetex.xstart, sidetex.yend,   coreColor});
			va->SafeAppend({pos1 + (ydir * muzzleCoreSize),                           sidetex.xstart, sidetex.ystart, coreColor});
		}

		pulseStartTime += 0.5f;
		pulseStartTime -= (1.0f * (pulseStartTime > 1.0f));

		for (int i = 0; i < 3; i++) {
			coreColor[i] = int(coreColStart[i] * (1.0f - pulseStartTime));
			edgeColor[i] = int(edgeColStart[i] * (1.0f - pulseStartTime));
		}

		muzzleEdgeSize = thickness * flaresize * pulseStartTime;

		{
			va->SafeAppend({pos1 + (ydir * muzzleEdgeSize),                           sidetex.xstart, sidetex.ystart, edgeColor});
			va->SafeAppend({pos1 + (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), sidetex.xend,   sidetex.ystart, edgeColor});
			va->SafeAppend({pos1 - (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), sidetex.xend,   sidetex.yend,   edgeColor});

			va->SafeAppend({pos1 - (ydir * muzzleEdgeSize) + (zdir * muzzleEdgeSize), sidetex.xend,   sidetex.yend,   edgeColor});
			va->SafeAppend({pos1 - (ydir * muzzleEdgeSize),                           sidetex.xstart, sidetex.yend,   edgeColor});
			va->SafeAppend({pos1 + (ydir * muzzleEdgeSize),                           sidetex.xstart, sidetex.ystart, edgeColor});
		}

		muzzleCoreSize = muzzleEdgeSize * 0.6f;

		{
			va->SafeAppend({pos1 + (ydir * muzzleCoreSize),                           sidetex.xstart, sidetex.ystart, coreColor});
			va->SafeAppend({pos1 + (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), sidetex.xend,   sidetex.ystart, coreColor});
			va->SafeAppend({pos1 - (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), sidetex.xend,   sidetex.yend,   coreColor});

			va->SafeAppend({pos1 - (ydir * muzzleCoreSize) + (zdir * muzzleCoreSize), sidetex.xend,   sidetex.yend,   coreColor});
			va->SafeAppend({pos1 - (ydir * muzzleCoreSize),                           sidetex.xstart, sidetex.yend,   coreColor});
			va->SafeAppend({pos1 + (ydir * muzzleCoreSize),                           sidetex.xstart, sidetex.ystart, coreColor});
		}
	}

	{
		// draw flare (moved slightly along the camera direction)
		pos1 = startPos - (camera->GetDir() * 3.0f);

		{
			va->SafeAppend({pos1 - (camR * flareEdgeSize) - (camU * flareEdgeSize), WT4->xstart, WT4->ystart, edgeColStart});
			va->SafeAppend({pos1 + (camR * flareEdgeSize) - (camU * flareEdgeSize), WT4->xend,   WT4->ystart, edgeColStart});
			va->SafeAppend({pos1 + (camR * flareEdgeSize) + (camU * flareEdgeSize), WT4->xend,   WT4->yend,   edgeColStart});

			va->SafeAppend({pos1 + (camR * flareEdgeSize) + (camU * flareEdgeSize), WT4->xend,   WT4->yend,   edgeColStart});
			va->SafeAppend({pos1 - (camR * flareEdgeSize) + (camU * flareEdgeSize), WT4->xstart, WT4->yend,   edgeColStart});
			va->SafeAppend({pos1 - (camR * flareEdgeSize) - (camU * flareEdgeSize), WT4->xstart, WT4->ystart, edgeColStart});
		}
		{
			va->SafeAppend({pos1 - (camR * flareCoreSize) - (camU * flareCoreSize), WT4->xstart, WT4->ystart, coreColStart});
			va->SafeAppend({pos1 + (camR * flareCoreSize) - (camU * flareCoreSize), WT4->xend,   WT4->ystart, coreColStart});
			va->SafeAppend({pos1 + (camR * flareCoreSize) + (camU * flareCoreSize), WT4->xend,   WT4->yend,   coreColStart});

			va->SafeAppend({pos1 + (camR * flareCoreSize) + (camU * flareCoreSize), WT4->xend,   WT4->yend,   coreColStart});
			va->SafeAppend({pos1 - (camR * flareCoreSize) + (camU * flareCoreSize), WT4->xstart, WT4->yend,   coreColStart});
			va->SafeAppend({pos1 - (camR * flareCoreSize) - (camU * flareCoreSize), WT4->xstart, WT4->ystart, coreColStart});
		}
	}

	#undef WT4
	#undef WT2
}

void CLargeBeamLaserProjectile::DrawOnMinimap(GL::RenderDataBufferC* va)
{
	const unsigned char color[4] = {edgeColStart[0], edgeColStart[1], edgeColStart[2], 255};

	va->SafeAppend({ startPos, color});
	va->SafeAppend({targetPos, color});
}

