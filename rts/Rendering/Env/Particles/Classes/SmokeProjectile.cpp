/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SmokeProjectile.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"

CR_BIND_DERIVED(CSmokeProjectile, CProjectile, )

CR_REG_METADATA(CSmokeProjectile,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(color),
		CR_MEMBER(size),
		CR_MEMBER(startSize),
		CR_MEMBER(sizeExpansion),
		CR_MEMBER(ageSpeed),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_MEMBER(age),
	CR_MEMBER(textureNum)
))


CSmokeProjectile::CSmokeProjectile():
	color(0.5f),
	age(0.0f),
	ageSpeed(1.0f),
	size(0.0f),
	startSize(0.0f),
	sizeExpansion(0.0f),
	textureNum(0)
{
	checkCol = false;
}

CSmokeProjectile::CSmokeProjectile(
	CUnit* owner,
	const float3& pos,
	const float3& speed,
	float ttl,
	float startSize,
	float sizeExpansion,
	float color
):
	CProjectile(pos, speed, owner, false, false, false),

	color(color),
	age(0),
	size(0),
	startSize(startSize),
	sizeExpansion(sizeExpansion)
{
	ageSpeed = 1.0f / ttl;
	checkCol = false;
	castShadow = true;
	textureNum = (int) (guRNG.NextInt(projectileDrawer->NumSmokeTextures()));

	useAirLos |= ((pos.y - CGround::GetApproximateHeight(pos.x, pos.z, false)) > 10.0f);
	alwaysVisible |= (owner == nullptr);
}



void CSmokeProjectile::Init(const CUnit* owner, const float3& offset)
{
	textureNum = (int) (guRNG.NextInt(projectileDrawer->NumSmokeTextures()));

	useAirLos |= (offset.y - CGround::GetApproximateHeight(offset.x, offset.z, false) > 10.0f);
	alwaysVisible |= (owner == nullptr);

	CProjectile::Init(owner, offset);
}

void CSmokeProjectile::Update()
{
	pos += speed;
	pos += (envResHandler.GetCurrentWindVec() * age * 0.05f);
	age += ageSpeed;
	size += sizeExpansion;
	size += ((startSize - size) * 0.2f * (size < startSize));

	drawRadius = size;
	age = std::min(age, 1.0f);

	deleteMe |= (age >= 1.0f);
}

void CSmokeProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	unsigned char col[4];
	unsigned char alpha = (unsigned char) ((1 - age) * 255);
	col[0] = (unsigned char) (color * alpha);
	col[1] = (unsigned char) (color * alpha);
	col[2] = (unsigned char) (color * alpha);
	col[3] = (unsigned char) alpha /* -alphaFalloff * globalRendering->timeOffset */;

	const float interSize = size + (sizeExpansion * globalRendering->timeOffset);
	const float3 pos1 ((camera->GetRight() - camera->GetUp()) * interSize);
	const float3 pos2 ((camera->GetRight() + camera->GetUp()) * interSize);

	#define st projectileDrawer->GetSmokeTexture(textureNum)
	va->SafeAppend({drawPos - pos2, st->xstart, st->ystart, col});
	va->SafeAppend({drawPos + pos1, st->xend,   st->ystart, col});
	va->SafeAppend({drawPos + pos2, st->xend,   st->yend,   col});

	va->SafeAppend({drawPos + pos2, st->xend,   st->yend,   col});
	va->SafeAppend({drawPos - pos1, st->xstart, st->yend,   col});
	va->SafeAppend({drawPos - pos2, st->xstart, st->ystart, col});
	#undef st
}


bool CSmokeProjectile::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_FLOAT (CSmokeProjectile, color        )
	CHECK_MEMBER_INFO_FLOAT (CSmokeProjectile, size         )
	CHECK_MEMBER_INFO_FLOAT (CSmokeProjectile, startSize    )
	CHECK_MEMBER_INFO_FLOAT (CSmokeProjectile, sizeExpansion)
	CHECK_MEMBER_INFO_FLOAT (CSmokeProjectile, ageSpeed     )

	return false;
}
