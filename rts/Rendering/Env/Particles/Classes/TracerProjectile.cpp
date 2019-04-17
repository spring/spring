/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TracerProjectile.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"

CR_BIND_DERIVED(CTracerProjectile, CProjectile, )

CR_REG_METADATA(CTracerProjectile,
(
	CR_MEMBER(speedf),
	CR_MEMBER(drawLength),
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(length),
	CR_MEMBER_ENDFLAG(CM_Config)
))


CTracerProjectile::CTracerProjectile()
	: speedf(0.0f)
	, length(0.0f)
	, drawLength(0.0f)
{
	checkCol = false;
}

CTracerProjectile::CTracerProjectile(CUnit* owner, const float3& pos, const float3& spd, const float range)
	: CProjectile(pos, spd, owner, false, false, false)
	, length(range)
	, drawLength(0.0f)
{
	SetRadiusAndHeight(1.0f, 0.0f);

	checkCol = false;
	// Projectile::Init has been called by base ctor, so .w is defined
	// FIXME: constant, assumes |speed| never changes after creation
	speedf = this->speed.w;
}

void CTracerProjectile::Init(const CUnit* owner, const float3& offset)
{
	CProjectile::Init(owner, offset);

	// FIXME: constant,assumes |speed| never changes after creation
	speedf = speed.w;
}



void CTracerProjectile::Update()
{
	pos += speed;

	drawLength += speedf;
	drawLength = std::min(drawLength, 3.0f);
	length -= speedf;

	deleteMe |= (length < 0.0f);
}

void CTracerProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	// semi-degenerate quad; emulates a line
	va->SafeAppend({drawPos + UpVector * 0.001f                   ,  1.0f / 16, 1.0f / 16,  {1.0f, 1.0f, 0.1f, 0.4f}});
	va->SafeAppend({drawPos + UpVector * 0.001f - dir * drawLength,  1.0f / 16, 1.0f / 16,  {1.0f, 1.0f, 0.1f, 0.4f}});
	va->SafeAppend({drawPos - UpVector * 0.001f - dir * drawLength,  1.0f / 16, 1.0f / 16,  {1.0f, 1.0f, 0.1f, 0.4f}});

	va->SafeAppend({drawPos - UpVector * 0.001f - dir * drawLength,  1.0f / 16, 1.0f / 16,  {1.0f, 1.0f, 0.1f, 0.4f}});
	va->SafeAppend({drawPos - UpVector * 0.001f                   ,  1.0f / 16, 1.0f / 16,  {1.0f, 1.0f, 0.1f, 0.4f}});
	va->SafeAppend({drawPos + UpVector * 0.001f                   ,  1.0f / 16, 1.0f / 16,  {1.0f, 1.0f, 0.1f, 0.4f}});
}


bool CTracerProjectile::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_FLOAT (CTracerProjectile, length)

	return false;
}
