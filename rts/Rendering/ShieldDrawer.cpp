/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "ShieldDrawer.h"
#include "Game/Camera.h"
#include "Lua/LuaRules.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/EventHandler.h"

unsigned int NumVisibleShields(const CUnit* u) {
	unsigned int n = 0;

	for (unsigned int i = 0; i < u->weapons.size(); i++) {
		const CWeapon* w = u->weapons[i];
		const WeaponDef* wd = w->weaponDef;

		if (!wd->isShield || !wd->visibleShield)
			continue;

		n += 1;
	}

	return n;
}

void AddUnitShieldSegments(const CUnit* u) {
	for (unsigned int i = 0; i < u->weapons.size(); i++) {
		const CWeapon* w = u->weapons[i];
		const WeaponDef* wd = w->weaponDef;

		if (!wd->isShield || !wd->visibleShield)
			continue;

		// XXX #1 (<u> cannot be non-const)
		CPlasmaRepulser* shield = const_cast<CPlasmaRepulser*>(static_cast<const CPlasmaRepulser*>(w));

		// 32 (4*8) parts
		for (int y = 0; y < 16; y += 4) {
			for (int x = 0; x < 32; x += 4) {
				shield->AddShieldSegment(new ShieldSegment(wd, u->pos, x, y));
			}
		}
	}
}

void DelUnitShieldSegments(const CUnit* u) {
	for (unsigned int i = 0; i < u->weapons.size(); i++) {
		const CWeapon* w = u->weapons[i];
		const WeaponDef* wd = w->weaponDef;

		if (!wd->isShield || !wd->visibleShield)
			continue;

		const CPlasmaRepulser* shield = static_cast<const CPlasmaRepulser*>(w);
		const std::list<ShieldSegment*>& shieldSegs = shield->GetShieldSegments();
		      std::list<ShieldSegment*>::const_iterator shieldSegsIt;

		for (shieldSegsIt = shieldSegs.begin(); shieldSegsIt != shieldSegs.end(); ++shieldSegsIt) {
			delete (*shieldSegsIt);
		}
	}
}



ShieldDrawer* shieldDrawer = NULL;

ShieldDrawer::ShieldDrawer(): CEventClient("[ShieldDrawer]", 271828 + 1, false) {
	eventHandler.AddClient(this);
}

ShieldDrawer::~ShieldDrawer() {
	eventHandler.RemoveClient(this);
}



void ShieldDrawer::RenderUnitCreated(const CUnit* u, int) {
	if (NumVisibleShields(u) == 0)
		return;

	units.insert(u);
	AddUnitShieldSegments(u);
}

void ShieldDrawer::RenderUnitDestroyed(const CUnit* u) {
	if (NumVisibleShields(u) == 0)
		return;

	units.erase(u);
	DelUnitShieldSegments(u);
}



void PushShieldDrawState() {
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1.0f, 1.0f, 1.0f, 0.2f);
	glAlphaFunc(GL_GREATER, 0.0f);
	glEnable(GL_ALPHA_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_TEXTURE_2D);

	projectileDrawer->textureAtlas->BindTexture();
}

void PopShieldDrawState() {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_ALPHA_TEST);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDepthMask(GL_TRUE);
	glDisable(GL_TEXTURE_2D);
}

void ShieldDrawer::Draw() {
	if (units.empty())
		return;

	// NOTE: Lua shares this state by default (including texture-bindings)
	PushShieldDrawState();

	{
		CVertexArray* va = GetVertexArray();
		va->Initialize();

		for (std::set<const CUnit*>::const_iterator it = units.begin(); it != units.end(); ++it) {
			DrawUnitShields(*it, va);
		}

		// draw all individual segment-quads
		va->DrawArrayTC(GL_QUADS);
	}

	PopShieldDrawState();
}

void ShieldDrawer::DrawUnitShields(const CUnit* u, CVertexArray* va) {
	for (unsigned int i = 0; i < u->weapons.size(); i++) {
		const CWeapon* w = u->weapons[i];
		const WeaponDef* wd = w->weaponDef;

		if (luaRules && luaRules->DrawShield(u->id, w->weaponNum))
			continue;
		if (!wd->isShield || !wd->visibleShield)
			continue;
		if (!camera->InView(w->weaponPos, wd->shieldRadius * 2.0f))
			continue;

		if (u->stunned || u->beingBuilt) {
			// if the unit that emits this shield is stunned or not
			// yet finished, prevent the shield segments from being
			// drawn
			continue;
		}

		// XXX #2 (ShieldSegment::Draw cannot be const)
		DrawUnitShield(const_cast<CPlasmaRepulser*>(static_cast<const CPlasmaRepulser*>(w)), va);
	}
}

void ShieldDrawer::DrawUnitShield(CPlasmaRepulser* shield, CVertexArray* va) {
	std::list<ShieldSegment*>& shieldSegs = shield->GetShieldSegments();
	std::list<ShieldSegment*>::iterator shieldSegsIt;

	if (shieldSegs.empty())
		return;

	// TODO: z-sort the segments?
	for (shieldSegsIt = shieldSegs.begin(); shieldSegsIt != shieldSegs.end(); ++shieldSegsIt) {
		(*shieldSegsIt)->Draw(shield, va);
	}
}






ShieldSegment::ShieldSegment(
	const WeaponDef* shieldWeaponDef,
	const float3& shieldSegmentPos,
	const int xpart,
	const int ypart
) {
	segmentPos = shieldSegmentPos;
	segmentColor = shieldWeaponDef->shieldBadColor;

	segmentSize = shieldWeaponDef->shieldRadius;
	segmentAlpha = shieldWeaponDef->shieldAlpha;

	texture = shieldWeaponDef->visuals.texture1;
	usePerlinTex = (texture == projectileDrawer->perlintex);

	if (texture != NULL) {
		const float xscale = (texture->xend - texture->xstart) * 0.25f, xmid = (texture->xstart + texture->xend) * 0.5f;
		const float yscale = (texture->yend - texture->ystart) * 0.25f, ymid = (texture->ystart + texture->yend) * 0.5f;

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
}

ShieldSegment::~ShieldSegment()
{
	if (projectileDrawer != NULL && usePerlinTex) {
		projectileDrawer->DecPerlinTexObjectCount();
	}
}


void ShieldSegment::Draw(CPlasmaRepulser* shield, CVertexArray* va)
{
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

