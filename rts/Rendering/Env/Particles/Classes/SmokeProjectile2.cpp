/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SmokeProjectile2.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"

CR_BIND_DERIVED(CSmokeProjectile2, CProjectile, )

CR_REG_METADATA(CSmokeProjectile2,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(color),
		CR_MEMBER(ageSpeed),
		CR_MEMBER(size),
		CR_MEMBER(startSize),
		CR_MEMBER(sizeExpansion),
		CR_MEMBER(wantedPos),
		CR_MEMBER(glowFalloff),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_MEMBER(age),
	CR_MEMBER(textureNum)
))


CSmokeProjectile2::CSmokeProjectile2():
	color(0.5f),
	age(0.0f),
	ageSpeed(1.0f),
	size(0.0f),
	startSize(0.0f),
	sizeExpansion(0.0f),
	textureNum(0),
	glowFalloff(0.0f)
{
	deleteMe = false;
	checkCol = false;
}

CSmokeProjectile2::CSmokeProjectile2(
	CUnit* owner,
	const float3& pos,
	const float3& wantedPos,
	const float3& speed,
	float ttl,
	float startSize,
	float sizeExpansion,
	float color)
: CProjectile(pos, speed, owner, false, false, false),
	color(color),
	age(0),
	size(0),
	startSize(startSize),
	sizeExpansion(sizeExpansion),
	wantedPos(wantedPos)
{
	ageSpeed = 1 / ttl;
	checkCol = false;
	castShadow = true;
	useAirLos |= ((pos.y - CGround::GetApproximateHeight(pos.x, pos.z, false)) > 10.0f);

	glowFalloff = 4.5f + guRNG.NextFloat() * 6;
	textureNum = (int)(guRNG.NextInt(projectileDrawer->NumSmokeTextures()));
}



void CSmokeProjectile2::Init(const CUnit* owner, const float3& offset)
{
	useAirLos |= (offset.y - CGround::GetApproximateHeight(offset.x, offset.z, false) > 10.0f);
	alwaysVisible |= (owner == nullptr);

	wantedPos += offset;

	CProjectile::Init(owner, offset);
}

void CSmokeProjectile2::Update()
{
	wantedPos += speed;
	wantedPos += (envResHandler.GetCurrentWindVec() * age * 0.05f);

	pos.x += (wantedPos.x - pos.x) * 0.07f;
	pos.y += (wantedPos.y - pos.y) * 0.02f;
	pos.z += (wantedPos.z - pos.z) * 0.07f;

	age += ageSpeed;
	size += sizeExpansion;
	size += ((startSize - size) * 0.2f * (size < startSize));

	SetRadiusAndHeight(size, 0.0f);
	age = std::min(age, 1.0f);

	deleteMe |= (age >= 1.0f);
}

void CSmokeProjectile2::Draw(GL::RenderDataBufferTC* va) const
{
	const float interAge = std::min(1.0f, age + ageSpeed * globalRendering->timeOffset);
	unsigned char col[4];
	unsigned char alpha;

	if (interAge < 0.05f) {
		alpha = (unsigned char) (     interAge  * 19 * 127);
	} else {
		alpha = (unsigned char) ((1 - interAge)      * 127);
	}

	const float rglow = std::max(0.f, (1 - (interAge * glowFalloff))        * 127);
	const float gglow = std::max(0.f, (1 - (interAge * glowFalloff * 2.5f)) * 127);

	col[0] = (unsigned char) (color * alpha + rglow);
	col[1] = (unsigned char) (color * alpha + gglow);
	col[2] = (unsigned char) std::max(0.f, color * alpha - gglow * 0.5f);
	col[3] = alpha;

	const float3 interPos = pos + (wantedPos + speed * globalRendering->timeOffset - pos) * 0.1f * globalRendering->timeOffset;
	const float interSize = size + sizeExpansion * globalRendering->timeOffset;
	const float3 pos1 ((camera->GetRight() - camera->GetUp()) * interSize);
	const float3 pos2 ((camera->GetRight() + camera->GetUp()) * interSize);

	#define st projectileDrawer->GetSmokeTexture(textureNum)
	va->SafeAppend({interPos - pos2, st->xstart, st->ystart, col});
	va->SafeAppend({interPos + pos1, st->xend,   st->ystart, col});
	va->SafeAppend({interPos + pos2, st->xend,   st->yend,   col});

	va->SafeAppend({interPos + pos2, st->xend,   st->yend,   col});
	va->SafeAppend({interPos - pos1, st->xstart, st->yend,   col});
	va->SafeAppend({interPos - pos2, st->xstart, st->ystart, col});
	#undef st
}


bool CSmokeProjectile2::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_FLOAT (CSmokeProjectile2, color        )
	CHECK_MEMBER_INFO_FLOAT (CSmokeProjectile2, size         )
	CHECK_MEMBER_INFO_FLOAT (CSmokeProjectile2, startSize    )
	CHECK_MEMBER_INFO_FLOAT (CSmokeProjectile2, sizeExpansion)
	CHECK_MEMBER_INFO_FLOAT (CSmokeProjectile2, ageSpeed     )
	CHECK_MEMBER_INFO_FLOAT (CSmokeProjectile2, glowFalloff  )
	CHECK_MEMBER_INFO_FLOAT3(CSmokeProjectile2, wantedPos    )

	return false;
}
