/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ShieldSegmentProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/EventHandler.h"
#include "System/SpringMath.h"
#include "System/UnorderedMap.hpp"


CR_BIND(ShieldSegmentCollection, )
CR_REG_METADATA(ShieldSegmentCollection, (
	CR_MEMBER(shield),
	CR_IGNORED(shieldTexture),
	CR_IGNORED(lastAllowDrawFrame),
	CR_IGNORED(allowDrawing),
	CR_MEMBER(shieldSegments),
	CR_MEMBER(color),
	CR_MEMBER(size),

	CR_POSTLOAD(PostLoad)
))


CR_BIND_DERIVED(ShieldSegmentProjectile, CProjectile, )

CR_REG_METADATA(ShieldSegmentProjectile, (
	CR_IGNORED(collection),
	CR_IGNORED(vertices),
	CR_IGNORED(texCoors)
))


static std::vector<float3> spherevertices;
static spring::unsynced_map<const AtlasedTexture*, std::vector<float2> > spheretexcoords;



ShieldSegmentCollection& ShieldSegmentCollection::operator=(ShieldSegmentCollection&& ssc) noexcept {
	shield = ssc.shield; ssc.shield = nullptr;
	shieldTexture = ssc.shieldTexture; ssc.shieldTexture = nullptr;

	lastAllowDrawFrame = ssc.lastAllowDrawFrame;
	allowDrawing = ssc.allowDrawing;

	size = ssc.size;
	color = ssc.color;

	shieldSegments = std::move(ssc.shieldSegments);

	// update collection-pointers if we are moved into
	for (ShieldSegmentProjectile* ssp: shieldSegments) {
		ssp->Reload(this, -1, -1);
	}

	return *this;
}


void ShieldSegmentCollection::Init(CPlasmaRepulser* shield_)
{
	shield = shield_;
	shieldTexture = nullptr;

	lastAllowDrawFrame = -1;
	allowDrawing = false;

	size = shield->weaponDef->shieldRadius;
	color = SColor(255, 255, 255, 0);


	const CUnit* u = shield->owner;
	const WeaponDef* wd = shield->weaponDef;

	if ((allowDrawing = wd->IsVisibleShield())) {
		shieldTexture = wd->visuals.texture1;

		// Y*X segments, deleted by ProjectileHandler
		for (int y = 0; y < NUM_SEGMENTS_Y; ++y) {
			for (int x = 0; x < NUM_SEGMENTS_X; ++x) {
				shieldSegments.push_back(projMemPool.alloc<ShieldSegmentProjectile>(this, wd, u->pos, x, y));
			}
		}

		// ProjectileDrawer needs to know if any shields use the Perlin-noise texture
		if (!UsingPerlinNoise())
			return;

		projectileDrawer->IncPerlinTexObjectCount();
	}
}

void ShieldSegmentCollection::Kill()
{
	{
		shield = nullptr;
		shieldTexture = nullptr;
	}
	{
		for (ShieldSegmentProjectile* seg: shieldSegments) {
			seg->PreDelete();
		}

		shieldSegments.clear();
	}
	{
		if (!UsingPerlinNoise())
			return;

		projectileDrawer->DecPerlinTexObjectCount();
	}
}


void ShieldSegmentCollection::PostLoad()
{
	lastAllowDrawFrame = -1;
	if (shield == nullptr)
		return;

	const WeaponDef* wd = shield->weaponDef;

	if ((allowDrawing = wd->IsVisibleShield())) {
		shieldTexture = wd->visuals.texture1;

		int i = 0;
		for (int y = 0; y < NUM_SEGMENTS_Y; ++y) {
			for (int x = 0; x < NUM_SEGMENTS_X; ++x) {
				shieldSegments[i++]->Reload(this, x, y);
			}
		}
	}
}


bool ShieldSegmentCollection::UsingPerlinNoise() const
{
	return (projectileDrawer != nullptr && shieldTexture == projectileDrawer->perlintex);
}

bool ShieldSegmentCollection::AllowDrawing()
{
	// call eventHandler.DrawShield only once per shield & frame
	if (lastAllowDrawFrame == globalRendering->drawFrame)
		return allowDrawing;

	lastAllowDrawFrame = globalRendering->drawFrame;


	if (shield == nullptr)
		return (allowDrawing = false);
	if (shield->owner == nullptr)
		return (allowDrawing = false);

	//FIXME if Lua wants to draw the shield itself, we should draw all GL_QUADS in the `va` vertexArray first.
	// but doing so for each shield might reduce the performance.
	// so might use a branch-predicion? -> save last return value and if it is true draw `va` before calling eventHandler.DrawShield()
	if (eventHandler.DrawShield(shield->owner, shield))
		return (allowDrawing = false);

	if (!shield->IsActive())
		return (allowDrawing = false);
	if (shieldSegments.empty())
		return (allowDrawing = false);
	if (!camera->InView(GetShieldDrawPos(), shield->weaponDef->shieldRadius * 2.0f))
		return (allowDrawing = false);

	// signal the ShieldSegmentProjectile's they can draw
	return (allowDrawing = true);
}

void ShieldSegmentCollection::UpdateColor()
{
	const WeaponDef* shieldDef = shield->weaponDef;

	// lerp between badColor and goodColor based on shield's current power
	const float relPower = shield->GetCurPower() / std::max(1.0f, shieldDef->shieldPower);
	const float lrpColor = std::min(1.0f, relPower);

	float alpha = shieldDef->visibleShield * shieldDef->shieldAlpha;

	if (shield->GetHitFrames() > 0 && shieldDef->visibleShieldHitFrames > 0) {
		// when a shield is hit, increase segment's opacity to
		// min(1, def->alpha + k * def->alpha) with k in [1, 0]
		alpha += (shield->GetHitFrames() / float(shieldDef->visibleShieldHitFrames)) * shieldDef->shieldAlpha;
		alpha = std::min(alpha, 1.0f);
	}

	color = SColor(mix(shieldDef->shieldBadColor, shieldDef->shieldGoodColor, lrpColor) * alpha);
}


float3 ShieldSegmentCollection::GetShieldDrawPos() const
{
	assert(shield != nullptr);
	assert(shield->owner != nullptr);
	return shield->owner->GetObjectSpaceDrawPos(shield->relWeaponMuzzlePos);
}


ShieldSegmentProjectile::ShieldSegmentProjectile(
	ShieldSegmentCollection* collection_,
	const WeaponDef* shieldWeaponDef,
	const float3& shieldSegmentPos,
	int xpart,
	int ypart
)
	: CProjectile(
		shieldSegmentPos,
		ZeroVector,
		collection_->GetShield()->owner,
		false,
		false,
		false
	)
	, collection(collection_)
{
	checkCol      = false;
	deleteMe      = false;
	alwaysVisible = false;
	useAirLos     = true;
	drawRadius    = shieldWeaponDef->shieldRadius * 0.4f;
	mygravity     = 0.0f;

	vertices = GetSegmentVertices(xpart, ypart);
	texCoors = GetSegmentTexCoords(collection->GetShieldTexture(), xpart, ypart);
}

void ShieldSegmentProjectile::Reload(ShieldSegmentCollection* collection_, int xpart, int ypart)
{
	assert(!deleteMe);

	collection = collection_;

	if (xpart < 0 && ypart < 0)
		return;

	vertices = GetSegmentVertices(xpart, ypart);
	texCoors = GetSegmentTexCoords(collection->GetShieldTexture(), xpart, ypart);
}


const float3* ShieldSegmentProjectile::GetSegmentVertices(const int xpart, const int ypart)
{
	if (spherevertices.empty()) {
		spherevertices.resize(ShieldSegmentCollection::NUM_SEGMENTS_Y * ShieldSegmentCollection::NUM_SEGMENTS_X * NUM_VERTICES_Y * NUM_VERTICES_X);

		#define NUM_VERTICES_X_M1 (NUM_VERTICES_X - 1)
		#define NUM_VERTICES_Y_M1 (NUM_VERTICES_Y - 1)

		// add <NUM_SEGMENTS_Y * NUM_SEGMENTS_X * NUM_VERTICES_Y * NUM_VERTICES_X> vertices
		for (int ypart_ = 0; ypart_ < ShieldSegmentCollection::NUM_SEGMENTS_Y; ++ypart_) {
			for (int xpart_ = 0; xpart_ < ShieldSegmentCollection::NUM_SEGMENTS_X; ++xpart_) {
				const int segmentIdx = (xpart_ + ypart_ * ShieldSegmentCollection::NUM_SEGMENTS_X) * (NUM_VERTICES_X * NUM_VERTICES_Y);
				for (int y = 0; y < NUM_VERTICES_Y; ++y) {
					const float yp = (y + ypart_ * NUM_VERTICES_Y_M1) / float(ShieldSegmentCollection::NUM_SEGMENTS_Y * NUM_VERTICES_Y_M1) * math::PI - math::HALFPI;
					for (int x = 0; x < NUM_VERTICES_X; ++x) {
						const float xp = (x + xpart_ * NUM_VERTICES_X_M1) / float(ShieldSegmentCollection::NUM_SEGMENTS_X * NUM_VERTICES_X_M1) * math::TWOPI;
						const size_t vIdx = segmentIdx + y * NUM_VERTICES_X + x;

						spherevertices[vIdx].x = std::sin(xp) * std::cos(yp);
						spherevertices[vIdx].y =                std::sin(yp);
						spherevertices[vIdx].z = std::cos(xp) * std::cos(yp);
					}
				}
			}
		}
	}

	const int segmentIdx = (xpart + ypart * ShieldSegmentCollection::NUM_SEGMENTS_X) * (NUM_VERTICES_X * NUM_VERTICES_Y);
	return &spherevertices[segmentIdx];
}

const float2* ShieldSegmentProjectile::GetSegmentTexCoords(const AtlasedTexture* texture, const int xpart, const int ypart)
{
	auto fit = spheretexcoords.find(texture);

	if (fit == spheretexcoords.end()) {
		std::vector<float2>& texcoords = spheretexcoords[texture];
		texcoords.resize(ShieldSegmentCollection::NUM_SEGMENTS_Y * ShieldSegmentCollection::NUM_SEGMENTS_X * NUM_VERTICES_Y * NUM_VERTICES_X);

		const float xscale = (texture == nullptr)? 0.0f: (texture->xend - texture->xstart) * 0.25f;
		const float yscale = (texture == nullptr)? 0.0f: (texture->yend - texture->ystart) * 0.25f;
		const float xmid   = (texture == nullptr)? 0.0f: (texture->xstart + texture->xend) * 0.5f;
		const float ymid   = (texture == nullptr)? 0.0f: (texture->ystart + texture->yend) * 0.5f;

		for (int ypart_ = 0; ypart_ < ShieldSegmentCollection::NUM_SEGMENTS_Y; ++ypart_) {
			for (int xpart_ = 0; xpart_ < ShieldSegmentCollection::NUM_SEGMENTS_X; ++xpart_) {
				const int segmentIdx = (xpart_ + ypart_ * ShieldSegmentCollection::NUM_SEGMENTS_X) * (NUM_VERTICES_X * NUM_VERTICES_Y);
				for (int y = 0; y < NUM_VERTICES_Y; ++y) {
					for (int x = 0; x < NUM_VERTICES_X; ++x) {
						const size_t vIdx = segmentIdx + y * NUM_VERTICES_X + x;

						texcoords[vIdx].x = (spherevertices[vIdx].x * (2 - std::fabs(spherevertices[vIdx].y))) * xscale + xmid;
						texcoords[vIdx].y = (spherevertices[vIdx].z * (2 - std::fabs(spherevertices[vIdx].y))) * yscale + ymid;
					}
				}
			}
		}

		fit = spheretexcoords.find(texture);
	}

	const int segmentIdx = (xpart + ypart * ShieldSegmentCollection::NUM_SEGMENTS_X) * (NUM_VERTICES_X * NUM_VERTICES_Y);
	return &fit->second[segmentIdx];
}

void ShieldSegmentProjectile::Update()
{
	if (collection == nullptr)
		return;

	// use the "middle" vertex for z-ordering
	pos = collection->GetShieldDrawPos() + vertices[(NUM_VERTICES_X * NUM_VERTICES_Y) >> 1] * collection->GetSize();

}

void ShieldSegmentProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	if (collection == nullptr)
		return;

	const SColor color = collection->GetColor();

	if (color.a == 0)
		return;

	if (!collection->AllowDrawing())
		return;


	const float3 shieldPos = collection->GetShieldDrawPos();
	const float size = collection->GetSize();

	// draw all quads
	for (int y = 0; y < (NUM_VERTICES_Y - 1); ++y) {
		for (int x = 0; x < (NUM_VERTICES_X - 1); ++x) {
			const int idxTL = (y    ) * NUM_VERTICES_X + x, idxTR = (y    ) * NUM_VERTICES_X + x + 1;
			const int idxBL = (y + 1) * NUM_VERTICES_X + x, idxBR = (y + 1) * NUM_VERTICES_X + x + 1;

			va->SafeAppend({shieldPos + vertices[idxTL] * size, texCoors[idxTL].x, texCoors[idxTL].y, color});
			va->SafeAppend({shieldPos + vertices[idxTR] * size, texCoors[idxTR].x, texCoors[idxTR].y, color});
			va->SafeAppend({shieldPos + vertices[idxBR] * size, texCoors[idxBR].x, texCoors[idxBR].y, color});

			va->SafeAppend({shieldPos + vertices[idxBR] * size, texCoors[idxBR].x, texCoors[idxBR].y, color});
			va->SafeAppend({shieldPos + vertices[idxBL] * size, texCoors[idxBL].x, texCoors[idxBL].y, color});
			va->SafeAppend({shieldPos + vertices[idxTL] * size, texCoors[idxTL].x, texCoors[idxTL].y, color});
		}
	}
}

