/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "BeamLaserProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

CR_BIND_DERIVED(CBeamLaserProjectile, CWeaponProjectile, (ProjectileParams(), 0.0f, 0.0f, float3(ZeroVector)));

CR_REG_METADATA(CBeamLaserProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(coreColStart),
	CR_MEMBER(coreColEnd),
	CR_MEMBER(edgeColStart),
	CR_MEMBER(edgeColEnd),
	CR_MEMBER(thickness),
	CR_MEMBER(corethickness),
	CR_MEMBER(flaresize),
	CR_MEMBER(decay),
	CR_MEMBER(midtexx),
	CR_RESERVED(16)
	));


CBeamLaserProjectile::CBeamLaserProjectile(const ProjectileParams& params, float startAlpha, float endAlpha, const float3& color)
	: CWeaponProjectile(params, true)
	, thickness(weaponDef? weaponDef->visuals.thickness: 0.0f)
	, corethickness(weaponDef? weaponDef->visuals.corethickness: 0.0f)
	, flaresize(weaponDef? weaponDef->visuals.laserflaresize: 0.0f)
	, decay(weaponDef? weaponDef->visuals.beamdecay: 0.0f)
{
	projectileType = WEAPON_BEAMLASER_PROJECTILE;
	checkCol = false;
	useAirLos = true;

	SetRadiusAndHeight(pos.distance(targetPos), 0.0f);

	if (weaponDef) {
		midtexx =
			(weaponDef->visuals.texture2->xstart +
			(weaponDef->visuals.texture2->xend - weaponDef->visuals.texture2->xstart) * 0.5f);

		coreColStart[0] = (weaponDef->visuals.color2.x * startAlpha);
		coreColStart[1] = (weaponDef->visuals.color2.y * startAlpha);
		coreColStart[2] = (weaponDef->visuals.color2.z * startAlpha);
		coreColStart[3] = 1;
		coreColEnd[0] = (weaponDef->visuals.color2.x * endAlpha);
		coreColEnd[1] = (weaponDef->visuals.color2.y * endAlpha);
		coreColEnd[2] = (weaponDef->visuals.color2.z * endAlpha);
		coreColEnd[3] = 1;
		edgeColStart[0] = (color.x * startAlpha);
		edgeColStart[1] = (color.y * startAlpha);
		edgeColStart[2] = (color.z * startAlpha);
		edgeColStart[3] = 1;
		edgeColEnd[0] = (color.x * endAlpha);
		edgeColEnd[1] = (color.y * endAlpha);
		edgeColEnd[2] = (color.z * endAlpha);
		edgeColEnd[3] = 1;
	} else {
		midtexx = 0.0f;
	}

	cegID = gCEG->Load(explGenHandler, (weaponDef != NULL)? weaponDef->cegTag: "");
}



void CBeamLaserProjectile::Update()
{
	ttl--;

	if (ttl <= 0) {
		deleteMe = true;
	} else {
		for (int i = 0; i < 3; i++) {
			coreColStart[i] *= decay;
			coreColEnd[i]   *= decay;
			edgeColStart[i] *= decay;
			edgeColEnd[i]   *= decay;
		}

		gCEG->Explosion(cegID, startpos + ((targetPos - startpos) / ttl), 0.0f, flaresize, 0x0, 0.0f, 0x0, targetPos - startpos);
	}

	UpdateInterception();
}

void CBeamLaserProjectile::Draw()
{
	inArray = true;

	const float3 cameraDir = (pos - camera->pos).SafeANormalize();
	// beam's coor-system; degenerate if targetPos == startPos
	const float3 zdir = (targetPos - startpos).SafeANormalize();
	const float3 xdir = (cameraDir.cross(zdir)).SafeANormalize();
	const float3 ydir = (cameraDir.cross(xdir));

	const float beamEdgeSize = thickness;
	const float beamCoreSize = beamEdgeSize * corethickness;
	const float flareEdgeSize = thickness * flaresize;
	const float flareCoreSize = flareEdgeSize * corethickness;

	const float3& pos1 = startpos;
	const float3& pos2 = targetPos;

	va->EnlargeArrays(32, 0, VA_SIZE_TC);

	#define WT1 weaponDef->visuals.texture1
	#define WT2 weaponDef->visuals.texture2
	#define WT3 weaponDef->visuals.texture3

	if ((pos - camera->pos).SqLength() < (1000.0f * 1000.0f)) {
		va->AddVertexQTC(pos1 - xdir * beamEdgeSize,                       midtexx,   WT2->ystart, edgeColStart);
		va->AddVertexQTC(pos1 + xdir * beamEdgeSize,                       midtexx,   WT2->yend,   edgeColStart);
		va->AddVertexQTC(pos1 + xdir * beamEdgeSize - ydir * beamEdgeSize, WT2->xend, WT2->yend,   edgeColStart);
		va->AddVertexQTC(pos1 - xdir * beamEdgeSize - ydir * beamEdgeSize, WT2->xend, WT2->ystart, edgeColStart);
		va->AddVertexQTC(pos1 - xdir * beamCoreSize,                       midtexx,   WT2->ystart, coreColStart);
		va->AddVertexQTC(pos1 + xdir * beamCoreSize,                       midtexx,   WT2->yend,   coreColStart);
		va->AddVertexQTC(pos1 + xdir * beamCoreSize - ydir * beamCoreSize, WT2->xend, WT2->yend,   coreColStart);
		va->AddVertexQTC(pos1 - xdir * beamCoreSize - ydir * beamCoreSize, WT2->xend, WT2->ystart, coreColStart);

		va->AddVertexQTC(pos1 - xdir * beamEdgeSize,                       WT1->xstart, WT1->ystart, edgeColStart);
		va->AddVertexQTC(pos1 + xdir * beamEdgeSize,                       WT1->xstart, WT1->yend,   edgeColStart);
		va->AddVertexQTC(pos2 + xdir * beamEdgeSize,                       WT1->xend,   WT1->yend,   edgeColEnd);
		va->AddVertexQTC(pos2 - xdir * beamEdgeSize,                       WT1->xend,   WT1->ystart, edgeColEnd);
		va->AddVertexQTC(pos1 - xdir * beamCoreSize,                       WT1->xstart, WT1->ystart, coreColStart);
		va->AddVertexQTC(pos1 + xdir * beamCoreSize,                       WT1->xstart, WT1->yend,   coreColStart);
		va->AddVertexQTC(pos2 + xdir * beamCoreSize,                       WT1->xend,   WT1->yend,   coreColEnd);
		va->AddVertexQTC(pos2 - xdir * beamCoreSize,                       WT1->xend,   WT1->ystart, coreColEnd);

		va->AddVertexQTC(pos2 - xdir * beamEdgeSize,                       midtexx,   WT2->ystart, edgeColStart);
		va->AddVertexQTC(pos2 + xdir * beamEdgeSize,                       midtexx,   WT2->yend,   edgeColStart);
		va->AddVertexQTC(pos2 + xdir * beamEdgeSize + ydir * beamEdgeSize, WT2->xend, WT2->yend,   edgeColStart);
		va->AddVertexQTC(pos2 - xdir * beamEdgeSize + ydir * beamEdgeSize, WT2->xend, WT2->ystart, edgeColStart);
		va->AddVertexQTC(pos2 - xdir * beamCoreSize,                       midtexx,   WT2->ystart, coreColStart);
		va->AddVertexQTC(pos2 + xdir * beamCoreSize,                       midtexx,   WT2->yend,   coreColStart);
		va->AddVertexQTC(pos2 + xdir * beamCoreSize + ydir * beamCoreSize, WT2->xend, WT2->yend,   coreColStart);
		va->AddVertexQTC(pos2 - xdir * beamCoreSize + ydir * beamCoreSize, WT2->xend, WT2->ystart, coreColStart);
	} else {
		va->AddVertexQTC(pos1 - xdir * beamEdgeSize,                       WT1->xstart, WT1->ystart, edgeColStart);
		va->AddVertexQTC(pos1 + xdir * beamEdgeSize,                       WT1->xstart, WT1->yend,   edgeColStart);
		va->AddVertexQTC(pos2 + xdir * beamEdgeSize,                       WT1->xend,   WT1->yend,   edgeColEnd);
		va->AddVertexQTC(pos2 - xdir * beamEdgeSize,                       WT1->xend,   WT1->ystart, edgeColEnd);
		va->AddVertexQTC(pos1 - xdir * beamCoreSize,                       WT1->xstart, WT1->ystart, coreColStart);
		va->AddVertexQTC(pos1 + xdir * beamCoreSize,                       WT1->xstart, WT1->yend,   coreColStart);
		va->AddVertexQTC(pos2 + xdir * beamCoreSize,                       WT1->xend,   WT1->yend,   coreColEnd);
		va->AddVertexQTC(pos2 - xdir * beamCoreSize,                       WT1->xend,   WT1->ystart, coreColEnd);
	}

	// draw flare
	va->AddVertexQTC(pos1 - camera->right * flareEdgeSize - camera->up * flareEdgeSize, WT3->xstart, WT3->ystart, edgeColStart);
	va->AddVertexQTC(pos1 + camera->right * flareEdgeSize - camera->up * flareEdgeSize, WT3->xend,   WT3->ystart, edgeColStart);
	va->AddVertexQTC(pos1 + camera->right * flareEdgeSize + camera->up * flareEdgeSize, WT3->xend,   WT3->yend,   edgeColStart);
	va->AddVertexQTC(pos1 - camera->right * flareEdgeSize + camera->up * flareEdgeSize, WT3->xstart, WT3->yend,   edgeColStart);

	va->AddVertexQTC(pos1 - camera->right * flareCoreSize - camera->up * flareCoreSize, WT3->xstart, WT3->ystart, coreColStart);
	va->AddVertexQTC(pos1 + camera->right * flareCoreSize - camera->up * flareCoreSize, WT3->xend,   WT3->ystart, coreColStart);
	va->AddVertexQTC(pos1 + camera->right * flareCoreSize + camera->up * flareCoreSize, WT3->xend,   WT3->yend,   coreColStart);
	va->AddVertexQTC(pos1 - camera->right * flareCoreSize + camera->up * flareCoreSize, WT3->xstart, WT3->yend,   coreColStart);

	#undef WT3
	#undef WT2
	#undef WT1
}

void CBeamLaserProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	const unsigned char color[4] = {edgeColStart[0], edgeColStart[1], edgeColStart[2], 255};

	lines.AddVertexQC(startpos, color);
	lines.AddVertexQC(targetPos, color);
}
