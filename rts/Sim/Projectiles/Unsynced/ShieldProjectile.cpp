/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ShieldProjectile.h"
#include "Game/Camera.h"
#include "Lua/LuaRules.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/myMath.h"

CR_BIND_DERIVED(ShieldProjectile, CProjectile, (NULL));
CR_REG_METADATA(ShieldProjectile, (
	CR_MEMBER(shield),
	CR_MEMBER(shieldTexture),
	CR_MEMBER(lastAllowDrawingUpdate),
	CR_MEMBER(allowDrawing),
	CR_MEMBER(shieldSegments)
));

CR_BIND_DERIVED(ShieldSegmentProjectile, CProjectile, (NULL, NULL, ZeroVector, 0, 0));
CR_REG_METADATA(ShieldSegmentProjectile, (
	CR_MEMBER(shieldProjectile),
	CR_MEMBER(segmentPos),
	CR_MEMBER(segmentColor),
	CR_IGNORED(vertices),
	CR_IGNORED(texCoors),
	CR_MEMBER(segmentSize),
	CR_MEMBER(segmentAlpha),
	CR_MEMBER(usePerlinTex)
));

static std::vector<float3> spherevertices;
static std::map<const AtlasedTexture*, std::vector<float2> > spheretexcoords;



#define NUM_SEGMENTS_X 8
#define NUM_SEGMENTS_Y 4

ShieldProjectile::ShieldProjectile(CPlasmaRepulser* shield_)
	: CProjectile(
			shield_->weaponPos, // pos
			ZeroVector, // speed
			shield_->owner, // owner
			false, // isSynced
			false, // isWeapon
			false  // isPiece
		)
	, shield(shield_)
	, shieldTexture(NULL)
	, lastAllowDrawingUpdate(-1)
	, allowDrawing(false)
{
	checkCol      = false;
	deleteMe      = false;
	alwaysVisible = false;
	useAirLos     = true;
	drawRadius    = 1.0f;
	mygravity     = 0.0f;

	const CUnit* u = shield->owner;
	const WeaponDef* wd = shield->weaponDef;

	if ((allowDrawing = (wd->visibleShield || wd->visibleShieldHitFrames > 0))) {
		shieldTexture = wd->visuals.texture1;

		// Y*X segments, deleted by ProjectileHandler
		for (int y = 0; y < NUM_SEGMENTS_Y; ++y) {
			for (int x = 0; x < NUM_SEGMENTS_X; ++x) {
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

void ShieldProjectile::Update() {
	if (shield == NULL)
		return;
	if (shield->owner == NULL)
		return;

	const CUnit* owner = shield->owner;

	pos = owner->drawPos +
		owner->frontdir * shield->relWeaponPos.z +
		owner->updir    * shield->relWeaponPos.y +
		owner->rightdir * shield->relWeaponPos.x;
}

bool ShieldProjectile::AllowDrawing() {
	// call luaRules->DrawShield only once per shield & frame
	if (lastAllowDrawingUpdate == globalRendering->drawFrame)
		return allowDrawing;

	lastAllowDrawingUpdate = globalRendering->drawFrame;
	allowDrawing = false;

	if (shield == NULL)
		return allowDrawing;
	if (shield->owner == NULL)
		return allowDrawing;

	//FIXME if Lua wants to draw the shield itself, we should draw all GL_QUADS in the `va` vertexArray first.
	// but doing so for each shield might reduce the performance.
	// so might use a branch-prediciton? -> save last return value and if it is true draw `va` before calling luaRules->DrawShield()
	if (luaRules && luaRules->DrawShield(shield->owner, shield))
		return allowDrawing;

	// interpolate shield position for drawing
	const CUnit* owner = shield->owner;
	pos = owner->drawPos +
		owner->frontdir * shield->relWeaponPos.z +
		owner->updir    * shield->relWeaponPos.y +
		owner->rightdir * shield->relWeaponPos.x;

	// if the unit that emits this shield is stunned or not
	// yet finished, prevent the shield segments from being
	// drawn
	if (shield->owner->IsStunned() || shield->owner->beingBuilt)
		return allowDrawing;
	if (shieldSegments.empty())
		return allowDrawing;
	if (!camera->InView(pos, shield->weaponDef->shieldRadius * 2.0f))
		return allowDrawing;

	// signal the ShieldSegmentProjectile's they can draw
	allowDrawing = true;
	return allowDrawing;
}






#define NUM_VERTICES_X 5
#define NUM_VERTICES_Y 5

ShieldSegmentProjectile::ShieldSegmentProjectile(
			ShieldProjectile* shieldProjectile_,
			const WeaponDef* shieldWeaponDef,
			const float3& shieldSegmentPos,
			const int xpart,
			const int ypart
		)
	: CProjectile(
			shieldSegmentPos,
			ZeroVector,
			shieldProjectile_->owner(),
			false,
			false,
			false
		)
	, shieldProjectile(shieldProjectile_)
	, segmentPos(shieldSegmentPos)
	, segmentColor(shieldWeaponDef->shieldBadColor)
	, segmentSize(shieldWeaponDef->shieldRadius)
	, segmentAlpha(shieldWeaponDef->shieldAlpha)
{
	checkCol      = false;
	deleteMe      = false;
	alwaysVisible = false;
	useAirLos     = true;
	drawRadius    = shieldWeaponDef->shieldRadius * 0.4f;
	mygravity     = 0.0f;

	#define texture shieldProjectile->GetShieldTexture()
	usePerlinTex = (texture == projectileDrawer->perlintex);

	vertices = GetSegmentVertices(xpart, ypart);
	texCoors = GetSegmentTexCoords(texture, xpart, ypart);

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


const float3* ShieldSegmentProjectile::GetSegmentVertices(const int xpart, const int ypart)
{
	if (spherevertices.empty()) {
		spherevertices.resize(NUM_SEGMENTS_Y * NUM_SEGMENTS_X * NUM_VERTICES_Y * NUM_VERTICES_X);

		// NUM_SEGMENTS_Y * NUM_SEGMENTS_X * NUM_VERTICES_Y * NUM_VERTICES_X vertices
		for (int ypart_ = 0; ypart_ < NUM_SEGMENTS_Y; ++ypart_) {
			for (int xpart_ = 0; xpart_ < NUM_SEGMENTS_X; ++xpart_) {
				const int segmentIdx = (xpart_ + ypart_ * NUM_SEGMENTS_X) * (NUM_VERTICES_X * NUM_VERTICES_Y);
				for (int y = 0; y < NUM_VERTICES_Y; ++y) {
					const float yp = (y + ypart_ * 4) / float(NUM_SEGMENTS_Y * 4) * PI - PI / 2;
					for (int x = 0; x < NUM_VERTICES_X; ++x) {
						const float xp = (x + xpart_ * 4) / float(NUM_SEGMENTS_X * 4) * 2 * PI;
						const size_t vIdx = segmentIdx + y * NUM_VERTICES_X + x;

						spherevertices[vIdx].x = math::sin(xp) * math::cos(yp);
						spherevertices[vIdx].y = math::sin(yp);
						spherevertices[vIdx].z = math::cos(xp) * math::cos(yp);
					}
				}
			}
		}
	}

	const int segmentIdx = (xpart + ypart * NUM_SEGMENTS_X) * (NUM_VERTICES_X * NUM_VERTICES_Y);
	return &spherevertices[segmentIdx];
}

const float2* ShieldSegmentProjectile::GetSegmentTexCoords(const AtlasedTexture* texture, const int xpart, const int ypart)
{
	std::map<const AtlasedTexture*, std::vector<float2> >::iterator fit = spheretexcoords.find(texture);
	if (fit == spheretexcoords.end()) {
		std::vector<float2>& texcoords = spheretexcoords[texture];
		texcoords.resize(NUM_SEGMENTS_Y * NUM_SEGMENTS_X * NUM_VERTICES_Y * NUM_VERTICES_X);

		const float xscale = (texture == NULL)? 0.0f: (texture->xend - texture->xstart) * 0.25f;
		const float yscale = (texture == NULL)? 0.0f: (texture->yend - texture->ystart) * 0.25f;
		const float xmid = (texture == NULL)? 0.0f: (texture->xstart + texture->xend) * 0.5f;
		const float ymid = (texture == NULL)? 0.0f: (texture->ystart + texture->yend) * 0.5f;

		for (int ypart_ = 0; ypart_ < NUM_SEGMENTS_Y; ++ypart_) {
			for (int xpart_ = 0; xpart_ < NUM_SEGMENTS_X; ++xpart_) {
				const int segmentIdx = (xpart_ + ypart_ * NUM_SEGMENTS_X) * (NUM_VERTICES_X * NUM_VERTICES_Y);
				for (int y = 0; y < NUM_VERTICES_Y; ++y) {
//					const float yp = (y + ypart_ * 4) / float(NUM_SEGMENTS_Y * 4) * PI - PI / 2;
					for (int x = 0; x < NUM_VERTICES_X; ++x) {
						const size_t vIdx = segmentIdx + y * NUM_VERTICES_X + x;

						texcoords[vIdx].x = (spherevertices[vIdx].x * (2 - math::fabs(spherevertices[vIdx].y))) * xscale + xmid;
						texcoords[vIdx].y = (spherevertices[vIdx].z * (2 - math::fabs(spherevertices[vIdx].y))) * yscale + ymid;
					}
				}
			}
		}

		fit = spheretexcoords.find(texture);
	}

	const int segmentIdx = (xpart + ypart * NUM_SEGMENTS_X) * (NUM_VERTICES_X * NUM_VERTICES_Y);
	return &fit->second[segmentIdx];
}

void ShieldSegmentProjectile::Update() {
	if (shieldProjectile == NULL)
		return;

	// use the "middle" vertex for z-ordering
	pos = shieldProjectile->pos + vertices[(NUM_VERTICES_X * NUM_VERTICES_Y) >> 1] * segmentSize;
}

void ShieldSegmentProjectile::Draw() {
	if (shieldProjectile == NULL)
		return;
	if (!shieldProjectile->AllowDrawing())
		return;

	const CPlasmaRepulser* shield = shieldProjectile->GetShield();
	if (shield == NULL)
		return;
	if (!shield->isEnabled)
		return;
	const WeaponDef* shieldDef = shield->weaponDef;

	// lerp between badColor and goodColor based on shield's current power
	const float colorMix = std::min(1.0f, shield->curPower / std::max(1.0f, shieldDef->shieldPower));

	segmentPos   = shieldProjectile->pos;
	segmentColor = mix(shieldDef->shieldBadColor, shieldDef->shieldGoodColor, colorMix);
	segmentAlpha = shieldDef->visibleShield * shieldDef->shieldAlpha;

	if (shield->hitFrames > 0 && shieldDef->visibleShieldHitFrames > 0) {
		// when a shield is hit, increase segment's opacity to
		// min(1, def->alpha + k * def->alpha) with k in [1, 0]
		segmentAlpha += (shield->hitFrames / float(shieldDef->visibleShieldHitFrames)) * shieldDef->shieldAlpha;
		segmentAlpha = std::min(segmentAlpha, 1.0f);
	}

	if (segmentAlpha <= 0.0f)
		return;

	const unsigned char segmentColorRGBA[4] = {
		(unsigned char)( segmentColor.x * segmentAlpha * 255 ),
		(unsigned char)( segmentColor.y * segmentAlpha * 255 ),
		(unsigned char)( segmentColor.z * segmentAlpha * 255 ),
		(unsigned char)(                  segmentAlpha * 255 ),
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

