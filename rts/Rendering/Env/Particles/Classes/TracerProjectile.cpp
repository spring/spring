/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TracerProjectile.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderBuffers.h"
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
	length -= speedf;

	deleteMe |= (length < 0.0f);
}

void CTracerProjectile::Draw()
{
	drawLength = std::min(drawLength, 3.0f);

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_TC>();
	auto& sh = rb.GetShader();

	SColor col = SColor{ 1.0f, 1.0f, 0.1f, 0.4f };
	const float t = 1.0f / 16.0f;

	rb.AddVertex({ drawPos                   , t, t, col });
	rb.AddVertex({ drawPos - dir * drawLength, t, t, col });

	sh.Enable();
	rb.DrawArrays(GL_LINES);
	sh.Disable();
}

int CTracerProjectile::GetProjectilesCount() const
{
	return 100; // glBeginEnd is ways more evil than VA draw!
}

bool CTracerProjectile::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_FLOAT (CTracerProjectile, length)

	return false;
}
