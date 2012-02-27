/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ShieldProjectile.h"
#include "Game/Camera.h"
#include "Lua/LuaRules.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/myMath.h"

CR_BIND_DERIVED(ShieldProjectile, CProjectile, (NULL));
CR_BIND_DERIVED(ShieldSegmentProjectile, CProjectile, (NULL, NULL, ZeroVector, 0, 0));



#define NUM_SEGMENTS_X 8
#define NUM_SEGMENTS_Y 4

ShieldProjectile::ShieldProjectile(
	const CPlasmaRepulser* shield_
): CProjectile(
	ZeroVector,
	ZeroVector,
	shield_->owner,
	false,
	false,
	false
) {
	shield = shield_;
	shieldTexture = NULL;

	checkCol      = false;
	deleteMe      = false;
	alwaysVisible = false;
	useAirLos     = true;

	drawRadius = 1.0f;
	mygravity = 0.0f;

	const CUnit* u = shield->owner;
	const WeaponDef* wd = shield->weaponDef;

	if ((allowDrawing = wd->visibleShield)) {
		shieldTexture = wd->visuals.texture1;

		// Y*X segments, deleted by ProjectileHandler
		for (int y = 0; y < (NUM_SEGMENTS_Y * 4); y += 4) {
			for (int x = 0; x < (NUM_SEGMENTS_X * 4); x += 4) {
				shieldSegments.push_back(new ShieldSegmentProjectile(this, wd, u->pos, x, y));
			}
		}
	}
}

ShieldProjectile::~ShieldProjectile() {
	std::list<ShieldSegmentProjectile*>::const_iterator shieldSegsIt;

	for (shieldSegsIt = shieldSegments.begin(); shieldSegsIt != shieldSegments.end(); ++shieldSegsIt) {
		(*shieldSegsIt)->PreDelete();
	}
}

void ShieldProjectile::Draw() {
	allowDrawing = false;

	if (shield == NULL)
		return;
	if (shield->owner == NULL)
		return;

	if (luaRules && luaRules->DrawShield(shield->owner->id, shield->weaponNum))
		return;

	// if the unit that emits this shield is stunned or not
	// yet finished, prevent the shield segments from being
	// drawn
	if (shield->owner->stunned || shield->owner->beingBuilt)
		return;
	if (shieldSegments.empty())
		return;
	if (!camera->InView(shield->weaponPos, shield->weaponDef->shieldRadius * 2.0f))
		return;

	// signal the ShieldSegmentProjectile's they can draw
	allowDrawing = true;
}






#define NUM_VERTICES_X 5
#define NUM_VERTICES_Y 5

ShieldSegmentProjectile::ShieldSegmentProjectile(
	const ShieldProjectile* shieldProjectile_,
	const WeaponDef* shieldWeaponDef,
	const float3& shieldSegmentPos,
	const int xpart,
	const int ypart
): CProjectile(
	shieldSegmentPos,
	ZeroVector,
	shieldProjectile_->owner(),
	false,
	false,
	false
) {
	shieldProjectile = shieldProjectile_;

	checkCol      = false;
	deleteMe      = false;
	alwaysVisible = false;
	useAirLos     = true;

	drawRadius = shieldWeaponDef->shieldRadius * 0.4f;
	mygravity = 0.0f;

	segmentPos = shieldSegmentPos;
	segmentColor = shieldWeaponDef->shieldBadColor;

	segmentSize = shieldWeaponDef->shieldRadius;
	segmentAlpha = shieldWeaponDef->shieldAlpha;

	#define texture shieldProjectile->GetShieldTexture()
	usePerlinTex = (texture == projectileDrawer->perlintex);

	if (texture != NULL) {
		const float xscale = (texture->xend - texture->xstart) * 0.25f, xmid = (texture->xstart + texture->xend) * 0.5f;
		const float yscale = (texture->yend - texture->ystart) * 0.25f, ymid = (texture->ystart + texture->yend) * 0.5f;

		// N*N vertices, (N-1)*(N-1) quads (a segment is not planar)
		for (int y = 0; y < NUM_VERTICES_Y; ++y) {
			const float yp = (y + ypart) / float(NUM_SEGMENTS_Y * 4) * PI - PI / 2;

			for (int x = 0; x < NUM_VERTICES_X; ++x) {
				const float xp = (x + xpart) / float(NUM_SEGMENTS_X * 4) * 2 * PI;
				const unsigned int vIdx = y * NUM_VERTICES_X + x;

				vertices[vIdx].x = sin(xp) * cos(yp);
				vertices[vIdx].y = sin(yp);
				vertices[vIdx].z = cos(xp) * cos(yp);
				texCoors[vIdx].x = (vertices[vIdx].x * (2 - fabs(vertices[vIdx].y))) * xscale + xmid;
				texCoors[vIdx].y = (vertices[vIdx].z * (2 - fabs(vertices[vIdx].y))) * yscale + ymid;
			}
		}

	}

	// ProjectileDrawer needs to know if any segments use the Perlin-noise texture
	if (projectileDrawer != NULL && usePerlinTex) {
		projectileDrawer->IncPerlinTexObjectCount();
	}

	#undef texture
}

ShieldSegmentProjectile::~ShieldSegmentProjectile()
{
	if (projectileDrawer != NULL && usePerlinTex) {
		projectileDrawer->DecPerlinTexObjectCount();
	}
}

void ShieldSegmentProjectile::Draw() {
	// use the "middle" vertex for z-ordering
	pos = segmentPos + vertices[(NUM_VERTICES_X * NUM_VERTICES_Y) >> 1] * segmentSize;

	if (shieldProjectile == NULL)
		return;
	if (!shieldProjectile->AllowDrawing())
		return;

	const CPlasmaRepulser* shield = shieldProjectile->GetShield();
	const WeaponDef* shieldDef = shield->weaponDef;

	// lerp between badColor and goodColor based on shield's current power
	const float colorMix = std::min(1.0f, shield->curPower / std::max(1.0f, shieldDef->shieldPower));

	segmentPos = shield->weaponPos;
	segmentColor = mix(shieldDef->shieldBadColor, shieldDef->shieldGoodColor, colorMix);
	segmentAlpha = shieldDef->shieldAlpha;

	if (shield->hitFrames > 0 && shieldDef->visibleShieldHitFrames > 0) {
		// when a shield is hit, increase segment's opacity to
		// min(1, def->alpha + k * def->alpha) with k in [1, 0]
		segmentAlpha += (shield->hitFrames / float(shieldDef->visibleShieldHitFrames));
		segmentAlpha *= shieldDef->shieldAlpha;
		segmentAlpha = std::min(segmentAlpha, 1.0f);
	}

	if (segmentAlpha <= 0.0f)
		return;
	if (!shield->isEnabled)
		return;

	const unsigned char segmentColorRGBA[4] = {
		segmentColor.x * segmentAlpha * 255,
		segmentColor.y * segmentAlpha * 255,
		segmentColor.z * segmentAlpha * 255,
		                 segmentAlpha * 255,
	};

	inArray = true;
	va->EnlargeArrays(4 * 4 * 4, 0, VA_SIZE_TC);

	// draw all quads
	for (int y = 0; y < (NUM_VERTICES_Y - 1); ++y) {
		for (int x = 0; x < (NUM_VERTICES_X - 1); ++x) {
			const int idxTL = (y    ) * NUM_VERTICES_X + x, idxTR = (y    ) * NUM_VERTICES_X + x + 1;
			const int idxBL = (y + 1) * NUM_VERTICES_X + x, idxBR = (y + 1) * NUM_VERTICES_X + x + 1;

			va->AddVertexQTC(segmentPos + vertices[idxTL] * segmentSize, texCoors[idxTL].x, texCoors[idxTL].y, segmentColorRGBA);
			va->AddVertexQTC(segmentPos + vertices[idxTR] * segmentSize, texCoors[idxTR].x, texCoors[idxTR].y, segmentColorRGBA);
			va->AddVertexQTC(segmentPos + vertices[idxBR] * segmentSize, texCoors[idxBR].x, texCoors[idxBR].y, segmentColorRGBA);
			va->AddVertexQTC(segmentPos + vertices[idxBL] * segmentSize, texCoors[idxBL].x, texCoors[idxBL].y, segmentColorRGBA);
		}
	}
}

