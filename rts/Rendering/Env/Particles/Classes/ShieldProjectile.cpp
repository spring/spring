/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ShieldProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/EventHandler.h"
#include "System/myMath.h"


CR_BIND_DERIVED(ShieldProjectile, CProjectile, )
CR_REG_METADATA(ShieldProjectile, (
	CR_MEMBER(shield),
	CR_MEMBER(shieldTexture),

	CR_IGNORED(shieldSegments),
	CR_IGNORED(color),

	CR_POSTLOAD(PostLoad)
))

// Segments don't have to be cregged since they can be regenerated on
// PostLoad

static std::vector<float3> spherevertices;
static std::map<const AtlasedTexture*, std::vector<float2> > spheretexcoords;



#define NUM_SEGMENTS_X 6
#define NUM_SEGMENTS_Y 4

#define NUM_VERTICES_X 5
#define NUM_VERTICES_Y 3

ShieldProjectile::ShieldProjectile(CPlasmaRepulser* shield_)
	: CProjectile(
			shield_->weaponMuzzlePos, // pos
			ZeroVector, // speed
			shield_->owner, // owner
			false, // isSynced
			false, // isWeapon
			false  // isPiece
		)
	, shield(shield_)
	, shieldTexture(NULL)
	, color(255,255,255,0)
{
	checkCol      = false;
	deleteMe      = false;
	alwaysVisible = false;
	useAirLos     = true;
	drawRadius    = shield_->weaponDef->shieldRadius;
	mygravity     = 0.0f;

	PostLoad();
}

ShieldProjectile::~ShieldProjectile()
{
	if (UsingPerlinNoise())
		projectileDrawer->DecPerlinTexObjectCount();
}

void ShieldProjectile::PostLoad()
{
	const WeaponDef* wd = shield->weaponDef;

	if (wd->visibleShield || wd->visibleShieldHitFrames > 0) {
		shieldTexture = wd->visuals.texture1;

		// Y*X segments, deleted by ProjectileHandler
		for (int y = 0; y < NUM_SEGMENTS_Y; ++y) {
			for (int x = 0; x < NUM_SEGMENTS_X; ++x) {
				shieldSegments.emplace_back(this, wd, x, y);
			}
		}
		if (UsingPerlinNoise())
			projectileDrawer->IncPerlinTexObjectCount();

	}

	UpdateColor();
}

void ShieldProjectile::PreDelete()
{
	deleteMe = true;
}

void ShieldProjectile::Update()
{
	pos = GetShieldDrawPos();
	UpdateColor();
}

void ShieldProjectile::UpdateColor()
{
	{
		color.a = 0;
		const WeaponDef* shieldDef = shield->weaponDef;

		// lerp between badColor and goodColor based on shield's current power
		const float colorMix = std::min(1.0f, shield->GetCurPower() / std::max(1.0f, shieldDef->shieldPower));

		const float3 colorf = mix(shieldDef->shieldBadColor, shieldDef->shieldGoodColor, colorMix);
		float alpha = shieldDef->visibleShield * shieldDef->shieldAlpha;

		if (shield->GetHitFrames() > 0 && shieldDef->visibleShieldHitFrames > 0) {
			// when a shield is hit, increase segment's opacity to
			// min(1, def->alpha + k * def->alpha) with k in [1, 0]
			alpha += (shield->GetHitFrames() / float(shieldDef->visibleShieldHitFrames)) * shieldDef->shieldAlpha;
			alpha = std::min(alpha, 1.0f);
		}

		color = SColor(
			colorf.x * alpha,
			colorf.y * alpha,
			colorf.z * alpha,
			alpha
		);
	}
}

float3 ShieldProjectile::GetShieldDrawPos() const
{
	if (shield == nullptr)
		return ZeroVector;
	if (shield->owner == nullptr)
		return ZeroVector;
	return shield->owner->GetObjectSpaceDrawPos(shield->relWeaponMuzzlePos);
}

bool ShieldProjectile::AllowDrawing()
{
	if (shield == NULL)
		return false;
	if (shield->owner == NULL)
		return false;

	//FIXME if Lua wants to draw the shield itself, we should draw all GL_QUADS in the `va` vertexArray first.
	// but doing so for each shield might reduce the performance.
	// so might use a branch-predicion? -> save last return value and if it is true draw `va` before calling eventHandler.DrawShield() ??FIXME
	if (eventHandler.DrawShield(shield->owner, shield))
		return false;

	if (!shield->IsActive())
		return false;
	if (shieldSegments.empty())
		return false;
	if (!camera->InView(pos, shield->weaponDef->shieldRadius * 2.0f))
		return false;

	// signal the ShieldSegmentProjectile's they can draw
	return true;
}

void ShieldProjectile::Draw()
{
	if (!AllowDrawing())
		return;pos = GetShieldDrawPos();

	inArray = true;
	for (ShieldSegment& seg: shieldSegments) {
		seg.Draw(va, pos, color);
	}
}


int ShieldProjectile::GetProjectilesCount() const
{
	return (NUM_VERTICES_Y - 1) * (NUM_VERTICES_X - 1) * NUM_SEGMENTS_X * NUM_SEGMENTS_Y;
}

bool ShieldProjectile::UsingPerlinNoise() const
{
	return projectileDrawer && (GetShieldTexture() == projectileDrawer->perlintex);
}



ShieldSegment::ShieldSegment(
			ShieldProjectile* shieldProjectile_,
			const WeaponDef* shieldWeaponDef,
			const int xpart,
			const int ypart
		)
	: shieldProjectile(shieldProjectile_)
	, segmentSize(shieldWeaponDef->shieldRadius)
{
	vertices = GetSegmentVertices(xpart, ypart);
	texCoors = GetSegmentTexCoords(shieldProjectile->GetShieldTexture(), xpart, ypart);
}

ShieldSegment::~ShieldSegment()
{ }

const float3* ShieldSegment::GetSegmentVertices(const int xpart, const int ypart)
{
	if (spherevertices.empty()) {
		spherevertices.resize(NUM_SEGMENTS_Y * NUM_SEGMENTS_X * NUM_VERTICES_Y * NUM_VERTICES_X);

		#define NUM_VERTICES_X_M1 (NUM_VERTICES_X - 1)
		#define NUM_VERTICES_Y_M1 (NUM_VERTICES_Y - 1)

		// NUM_SEGMENTS_Y * NUM_SEGMENTS_X * NUM_VERTICES_Y * NUM_VERTICES_X vertices
		for (int ypart_ = 0; ypart_ < NUM_SEGMENTS_Y; ++ypart_) {
			for (int xpart_ = 0; xpart_ < NUM_SEGMENTS_X; ++xpart_) {
				const int segmentIdx = (xpart_ + ypart_ * NUM_SEGMENTS_X) * (NUM_VERTICES_X * NUM_VERTICES_Y);
				for (int y = 0; y < NUM_VERTICES_Y; ++y) {
					const float yp = (y + ypart_ * NUM_VERTICES_Y_M1) / float(NUM_SEGMENTS_Y * NUM_VERTICES_Y_M1) * PI - PI / 2;
					for (int x = 0; x < NUM_VERTICES_X; ++x) {
						const float xp = (x + xpart_ * NUM_VERTICES_X_M1) / float(NUM_SEGMENTS_X * NUM_VERTICES_X_M1) * 2 * PI;
						const size_t vIdx = segmentIdx + y * NUM_VERTICES_X + x;

						spherevertices[vIdx].x = std::sin(xp) * std::cos(yp);
						spherevertices[vIdx].y =                 std::sin(yp);
						spherevertices[vIdx].z = std::cos(xp) * std::cos(yp);
					}
				}
			}
		}
	}

	const int segmentIdx = (xpart + ypart * NUM_SEGMENTS_X) * (NUM_VERTICES_X * NUM_VERTICES_Y);
	return &spherevertices[segmentIdx];
}

const float2* ShieldSegment::GetSegmentTexCoords(const AtlasedTexture* texture, const int xpart, const int ypart)
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

	const int segmentIdx = (xpart + ypart * NUM_SEGMENTS_X) * (NUM_VERTICES_X * NUM_VERTICES_Y);
	return &fit->second[segmentIdx];
}


void ShieldSegment::Draw(CVertexArray* va, const float3& shieldPos, const SColor& color)
{
	if (color.a == 0)
		return;

	va->EnlargeArrays(NUM_VERTICES_Y * NUM_VERTICES_X * 4, 0, VA_SIZE_TC);

	// draw all quads
	for (int y = 0; y < (NUM_VERTICES_Y - 1); ++y) {
		for (int x = 0; x < (NUM_VERTICES_X - 1); ++x) {
			const int idxTL = (y    ) * NUM_VERTICES_X + x, idxTR = (y    ) * NUM_VERTICES_X + x + 1;
			const int idxBL = (y + 1) * NUM_VERTICES_X + x, idxBR = (y + 1) * NUM_VERTICES_X + x + 1;

			va->AddVertexQTC(shieldPos + vertices[idxTL] * segmentSize, texCoors[idxTL].x, texCoors[idxTL].y, color);
			va->AddVertexQTC(shieldPos + vertices[idxTR] * segmentSize, texCoors[idxTR].x, texCoors[idxTR].y, color);
			va->AddVertexQTC(shieldPos + vertices[idxBR] * segmentSize, texCoors[idxBR].x, texCoors[idxBR].y, color);
			va->AddVertexQTC(shieldPos + vertices[idxBL] * segmentSize, texCoors[idxBL].x, texCoors[idxBL].y, color);
		}
	}
}
