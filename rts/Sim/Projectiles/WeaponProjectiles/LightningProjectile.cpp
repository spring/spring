/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "LightningProjectile.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

#ifdef TRACE_SYNC
	#include "System/Sync/SyncTracer.h"
#endif

CR_BIND_DERIVED(CLightningProjectile, CWeaponProjectile, (ProjectileParams()));

CR_REG_METADATA(CLightningProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(color),
	CR_MEMBER(displacements),
	CR_MEMBER(displacements2)
));

CLightningProjectile::CLightningProjectile(const ProjectileParams& params): CWeaponProjectile(params, true)
{
	projectileType = WEAPON_LIGHTNING_PROJECTILE;

	checkCol = false; // FIXME?
	drawRadius = pos.distance(targetPos);
	// SetRadiusAndHeight(pos.distance(targetPos), 0.0f);

	if (weaponDef != NULL)
		color = weaponDef->visuals.color;

	displacements[0] = 0.0f;
	displacements2[0] = 0.0f;

	for (size_t d = 1; d < displacements_size; ++d) {
		displacements[d]  = (gs->randFloat() - 0.5f) * drawRadius * 0.05f;
		displacements2[d] = (gs->randFloat() - 0.5f) * drawRadius * 0.05f;
	}

#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "] ";
	tracefile << params.pos.x << " " << params.pos.y << " " << params.pos.z << " ";
	tracefile << params.end.x << " " << params.end.y << " " << params.end.z << "\n";
#endif
}

void CLightningProjectile::Update()
{
	if (--ttl <= 0) {
		deleteMe = true;
	} else {
		gCEG->Explosion(cegID, startpos + ((targetPos - startpos) / ttl), 0.0f, displacements[0], NULL, 0.0f, NULL, targetPos - startpos);
	}

	for (size_t d = 1; d < displacements_size; ++d) {
		displacements[d]  += (gs->randFloat() - 0.5f) * 0.3f;
		displacements2[d] += (gs->randFloat() - 0.5f) * 0.3f;
	}

	UpdateInterception();
}

void CLightningProjectile::Draw()
{
	inArray = true;
	unsigned char col[4];
	col[0] = (unsigned char) (color.x * 255);
	col[1] = (unsigned char) (color.y * 255);
	col[2] = (unsigned char) (color.z * 255);
	col[3] = 1; //intensity*255;

	const float3 ddir = (targetPos - startpos).Normalize();
	const float3 dif  = (startpos - camera->pos).Normalize();
	const float3 dir1 = (dif.cross(ddir)).Normalize();
	float3 tempPos = startpos;

	va->EnlargeArrays(18 * 4, 0, VA_SIZE_TC);
	for (size_t d = 1; d < displacements_size-1; ++d) {
		float f = (d + 1) * 0.111f;

		#define WDV (&weaponDef->visuals)
		va->AddVertexQTC(tempPos + (dir1 * (displacements[d    ] + WDV->thickness)), WDV->texture1->xstart, WDV->texture1->ystart, col);
		va->AddVertexQTC(tempPos + (dir1 * (displacements[d    ] - WDV->thickness)), WDV->texture1->xstart, WDV->texture1->yend,   col);
		tempPos = (startpos * (1.0f - f)) + (targetPos * f);
		va->AddVertexQTC(tempPos + (dir1 * (displacements[d + 1] - WDV->thickness)), WDV->texture1->xend,   WDV->texture1->yend,   col);
		va->AddVertexQTC(tempPos + (dir1 * (displacements[d + 1] + WDV->thickness)), WDV->texture1->xend,   WDV->texture1->ystart, col);
		#undef WDV
	}

	tempPos = startpos;
	for (size_t d = 1; d < displacements_size-1; ++d) {
		float f = (d + 1) * 0.111f;

		#define WDV (&weaponDef->visuals)
		va->AddVertexQTC(tempPos + dir1 * (displacements2[d    ] + WDV->thickness), WDV->texture1->xstart, WDV->texture1->ystart, col);
		va->AddVertexQTC(tempPos + dir1 * (displacements2[d    ] - WDV->thickness), WDV->texture1->xstart, WDV->texture1->yend,   col);
		tempPos = startpos * (1.0f - f) + targetPos * f;
		va->AddVertexQTC(tempPos + dir1 * (displacements2[d + 1] - WDV->thickness), WDV->texture1->xend,   WDV->texture1->yend,   col);
		va->AddVertexQTC(tempPos + dir1 * (displacements2[d + 1] + WDV->thickness), WDV->texture1->xend,   WDV->texture1->ystart, col);
		#undef WDV
	}
}

void CLightningProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	const unsigned char lcolor[4] = {
		(unsigned char)(color[0] * 255),
		(unsigned char)(color[1] * 255),
		(unsigned char)(color[2] * 255),
		                           255
	};
	lines.AddVertexQC(startpos,  lcolor);
	lines.AddVertexQC(targetPos, lcolor);
}

