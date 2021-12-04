/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "GenericParticleProjectile.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/ColorMap.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "System/creg/DefTypes.h"

CR_BIND_DERIVED(CGenericParticleProjectile, CProjectile, )

CR_REG_METADATA(CGenericParticleProjectile,(
	CR_MEMBER(gravity),
	CR_MEMBER(texture),
	CR_MEMBER(colorMap),
	CR_MEMBER(directional),
	CR_MEMBER(life),
	CR_MEMBER(decayrate),
	CR_MEMBER(size),
	CR_MEMBER(airdrag),
	CR_MEMBER(sizeGrowth),
	CR_MEMBER(sizeMod)
))


CGenericParticleProjectile::CGenericParticleProjectile(const CUnit* owner, const float3& pos, const float3& speed)
	: CProjectile(pos, speed, owner, false, false, false)

	, gravity(ZeroVector)
	, texture(nullptr)
	, colorMap(nullptr)
	, directional(false)
	, life(0.0f)
	, decayrate(0.0f)
	, size(0.0f)
	, airdrag(0.0f)
	, sizeGrowth(0.0f)
	, sizeMod(0.0f)
{
	// set fields from super-classes
	useAirLos = true;
	checkCol  = false;
	deleteMe  = false;
}


void CGenericParticleProjectile::Update()
{
	SetPosition(pos + speed);
	SetVelocityAndSpeed((speed + gravity) * airdrag);

	life += decayrate;
	size = size * sizeMod + sizeGrowth;

	deleteMe |= (life > 1.0f);
}

void CGenericParticleProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	float3 xdir = camera->GetRight();
	float3 ydir = camera->GetUp();

	if (directional) {
		const float3 dif = (pos - camera->GetPos()).ANormalize();

		xdir = dif.cross(speed).ANormalize();
		ydir = dif.cross(xdir);
	}

	unsigned char color[4];
	colorMap->GetColor(color, life);

	va->SafeAppend({drawPos + (-xdir - ydir) * size, texture->xstart, texture->ystart, color});
	va->SafeAppend({drawPos + (-xdir + ydir) * size, texture->xend,   texture->ystart, color});
	va->SafeAppend({drawPos + ( xdir + ydir) * size, texture->xend,   texture->yend,   color});

	va->SafeAppend({drawPos + ( xdir + ydir) * size, texture->xend,   texture->yend,   color});
	va->SafeAppend({drawPos + ( xdir - ydir) * size, texture->xstart, texture->yend,   color});
	va->SafeAppend({drawPos + (-xdir - ydir) * size, texture->xstart, texture->ystart, color});
}

