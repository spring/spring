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

CR_BIND_DERIVED(ShieldProjectile, CProjectile, (NULL));
CR_BIND_DERIVED(ShieldSegmentProjectile, CProjectile, (NULL, NULL, ZeroVector, 0, 0));



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

		// 4*8 segments, deleted by ProjectileHandler
		for (int y = 0; y < 16; y += 4) {
			for (int x = 0; x < 32; x += 4) {
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

		// 5*5 vertices
		for (int y = 0; y < 5; ++y) {
			const float yp = (y + ypart) / 16.0f * PI - PI / 2;

			for (int x = 0; x < 5; ++x) {
				const float xp = (x + xpart) / 32.0f * 2 * PI;

				vertices[y * 5 + x] = float3(sin(xp) * cos(yp), sin(yp), cos(xp) * cos(yp));
				texCoors[y * 5 + x].x = (vertices[y * 5 + x].x * (2 - fabs(vertices[y * 5 + x].y))) * xscale + xmid;
				texCoors[y * 5 + x].y = (vertices[y * 5 + x].z * (2 - fabs(vertices[y * 5 + x].y))) * yscale + ymid;
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
	pos = segmentPos + vertices[12] * segmentSize;

	if (shieldProjectile == NULL)
		return;
	if (!shieldProjectile->AllowDrawing())
		return;

	const CPlasmaRepulser* shield = shieldProjectile->GetShield();
	const WeaponDef* shieldDef = shield->weaponDef;
	const unsigned char segmentColorRGBA[4] = {
		segmentColor.x * segmentAlpha * 255,
		segmentColor.y * segmentAlpha * 255,
		segmentColor.z * segmentAlpha * 255,
		                 segmentAlpha * 255,
	};

	float rawSegmentAlpha = 1.0f;

	if (shield->hitFrames > 0 && shieldDef->visibleShieldHitFrames > 0) {
		rawSegmentAlpha += (shield->hitFrames / float(shieldDef->visibleShieldHitFrames));
		rawSegmentAlpha = std::min(1.0f, rawSegmentAlpha * shieldDef->shieldAlpha);
	}

	if (rawSegmentAlpha > 0.0f) {
		const float colorMix = std::min(1.0f, shield->curPower / std::max(1.0f, shieldDef->shieldPower));
		const float3 color = (shieldDef->shieldGoodColor * colorMix) + (shieldDef->shieldBadColor * (1.0f - colorMix));

		segmentPos = shield->weaponPos;
		segmentColor = color;
		segmentAlpha = shield->isEnabled? rawSegmentAlpha: 0.0f;
	}

	if (segmentAlpha <= 0.0f)
		return;

	inArray = true;
	va->EnlargeArrays(4 * 4 * 4, 0, VA_SIZE_TC);

	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			va->AddVertexQTC(segmentPos + vertices[(y    ) * 5 + x    ] * segmentSize, texCoors[(y    ) * 5 + x    ].x, texCoors[(y    ) * 5 + x    ].y, segmentColorRGBA);
			va->AddVertexQTC(segmentPos + vertices[(y    ) * 5 + x + 1] * segmentSize, texCoors[(y    ) * 5 + x + 1].x, texCoors[(y    ) * 5 + x + 1].y, segmentColorRGBA);
			va->AddVertexQTC(segmentPos + vertices[(y + 1) * 5 + x + 1] * segmentSize, texCoors[(y + 1) * 5 + x + 1].x, texCoors[(y + 1) * 5 + x + 1].y, segmentColorRGBA);
			va->AddVertexQTC(segmentPos + vertices[(y + 1) * 5 + x    ] * segmentSize, texCoors[(y + 1) * 5 + x    ].x, texCoors[(y + 1) * 5 + x    ].y, segmentColorRGBA);
		}
	}
}

