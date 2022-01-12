/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SpherePartProjectile.h"

#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "System/SpringMath.h"

CR_BIND_DERIVED(CSpherePartProjectile, CProjectile, )

CR_REG_METADATA(CSpherePartProjectile, (
	CR_MEMBER(centerPos),
	CR_MEMBER(vectors),
	CR_MEMBER(color),
	CR_MEMBER(sphereSize),
	CR_MEMBER(expansionSpeed),
	CR_MEMBER(xbase),
	CR_MEMBER(ybase),
	CR_MEMBER(baseAlpha),
	CR_MEMBER(age),
	CR_MEMBER(ttl),
	CR_MEMBER(texx),
	CR_MEMBER(texy)
))


CSpherePartProjectile::CSpherePartProjectile(
	const CUnit* owner,
	const float3& centerPos,
	int xpart,
	int ypart,
	float expansionSpeed,
	float alpha,
	int ttl,
	const float3& color
):
	CProjectile(centerPos, ZeroVector, owner, false, false, false),
	centerPos(centerPos),
	color(color),
	sphereSize(expansionSpeed),
	expansionSpeed(expansionSpeed),
	xbase(xpart),
	ybase(ypart),
	baseAlpha(alpha),
	age(0),
	ttl(ttl)
{
	deleteMe = false;
	checkCol = false;

	for(int y = 0; y < 5; ++y) {
		const float yp = (y + ypart) / 16.0f*math::PI - math::HALFPI;
		for (int x = 0; x < 5; ++x) {
			float xp = (x + xpart) / 32.0f*math::TWOPI;
			vectors[y*5 + x] = float3(std::sin(xp)*std::cos(yp), std::sin(yp), std::cos(xp)*std::cos(yp));
		}
	}
	pos = centerPos+vectors[12] * sphereSize;

	drawRadius = 60;

	texx = projectileDrawer->sphereparttex->xstart + (projectileDrawer->sphereparttex->xend - projectileDrawer->sphereparttex->xstart) * 0.5f;
	texy = projectileDrawer->sphereparttex->ystart + (projectileDrawer->sphereparttex->yend - projectileDrawer->sphereparttex->ystart) * 0.5f;
}


void CSpherePartProjectile::Update()
{
	deleteMe |= ((age += 1) >= ttl);
	sphereSize += expansionSpeed;
	pos = centerPos + vectors[12] * sphereSize;
}

void CSpherePartProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	unsigned char colors[2][4];

	const float interSize = sphereSize + expansionSpeed * globalRendering->timeOffset;
	const float offsetAge = float(age + globalRendering->timeOffset);

	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			const float2 alpha = {
				(1.0f - std::min(1.0f, offsetAge / (float) ttl)) * (1.0f - std::fabs(y +     ybase - 8.0f) / 8.0f),
				(1.0f - std::min(1.0f, offsetAge / (float) ttl)) * (1.0f - std::fabs(y + 1 + ybase - 8.0f) / 8.0f)
			};

			colors[0][0] =  (unsigned char) (color.x * 255.0f * baseAlpha * alpha.x);
			colors[0][1] =  (unsigned char) (color.y * 255.0f * baseAlpha * alpha.x);
			colors[0][2] =  (unsigned char) (color.z * 255.0f * baseAlpha * alpha.x);
			colors[0][3] = ((unsigned char) (           40.0f * baseAlpha * alpha.x)) + 1;

			colors[1][0] =  (unsigned char) (color.x * 255.0f * baseAlpha * alpha.y);
			colors[1][1] =  (unsigned char) (color.y * 255.0f * baseAlpha * alpha.y);
			colors[1][2] =  (unsigned char) (color.z * 255.0f * baseAlpha * alpha.y);
			colors[1][3] = ((unsigned char) (           40.0f * baseAlpha * alpha.y)) + 1;

			va->SafeAppend({centerPos + vectors[(y    ) * 5 + x    ] * interSize, texx, texy, colors[0]});
			va->SafeAppend({centerPos + vectors[(y    ) * 5 + x + 1] * interSize, texx, texy, colors[0]});
			va->SafeAppend({centerPos + vectors[(y + 1) * 5 + x + 1] * interSize, texx, texy, colors[1]});

			va->SafeAppend({centerPos + vectors[(y + 1) * 5 + x + 1] * interSize, texx, texy, colors[1]});
			va->SafeAppend({centerPos + vectors[(y + 1) * 5 + x    ] * interSize, texx, texy, colors[1]});
			va->SafeAppend({centerPos + vectors[(y    ) * 5 + x    ] * interSize, texx, texy, colors[0]});
		}
	}
}


void CSpherePartProjectile::CreateSphere(const CUnit* owner, int ttl, float alpha, float expansionSpeed, float3 pos, float3 color)
{
	for (int y = 0; y < 16; y += 4) {
		for (int x = 0; x < 32; x += 4) {
			projMemPool.alloc<CSpherePartProjectile>(owner, pos, x, y, expansionSpeed, alpha, ttl, color);
		}
	}
}





CSpherePartSpawner::CSpherePartSpawner()
	: alpha(0.0f)
	, ttl(0)
	, expansionSpeed(0.0f)
	, color(ZeroVector)
{
}


CR_BIND_DERIVED(CSpherePartSpawner, CProjectile, )

CR_REG_METADATA(CSpherePartSpawner,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(alpha),
		CR_MEMBER(ttl),
		CR_MEMBER(expansionSpeed),
		CR_MEMBER(color),
	CR_MEMBER_ENDFLAG(CM_Config)
))

void CSpherePartSpawner::Init(const CUnit* owner, const float3& offset)
{
	CProjectile::Init(owner, offset);
	deleteMe = true;
	CSpherePartProjectile::CreateSphere(owner, ttl, alpha, expansionSpeed, pos, color);
}

int CSpherePartSpawner::GetProjectilesCount() const
{
	return 0;
}


bool CSpherePartSpawner::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_FLOAT (CSpherePartSpawner, alpha         )
	CHECK_MEMBER_INFO_FLOAT (CSpherePartSpawner, expansionSpeed)
	CHECK_MEMBER_INFO_INT   (CSpherePartSpawner, ttl           )
	CHECK_MEMBER_INFO_FLOAT3(CSpherePartSpawner, color         )

	return false;
}
