/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "NanoProjectile.h"

#include "Game/Camera.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Colors.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"
#include "Sim/Projectiles/ProjectileHandler.h"

CR_BIND_DERIVED(CNanoProjectile, CProjectile, )

CR_REG_METADATA(CNanoProjectile,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(deathFrame),
		CR_MEMBER(color),
	CR_MEMBER_ENDFLAG(CM_Config)
))


CNanoProjectile::CNanoProjectile()
{
	deathFrame = 0;
	color[0] = color[1] = color[2] = color[3] = 255;

	checkCol = false;
	drawSorted = false;
}

CNanoProjectile::CNanoProjectile(float3 pos, float3 speed, int lifeTime, SColor c)
	: CProjectile(pos, speed, nullptr, false, false, false)
	, deathFrame(gs->frameNum + lifeTime)
	, color(c)
{
	checkCol = false;
	drawSorted = false;
	drawRadius = 3;

	projectileHandler.currentNanoParticles += 1;
}

CNanoProjectile::~CNanoProjectile()
{
	projectileHandler.currentNanoParticles -= 1;
}


void CNanoProjectile::Update()
{
	pos += speed;
	deleteMe |= (gs->frameNum >= deathFrame);
}

void CNanoProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	const auto* gfxt = projectileDrawer->gfxtex;

	va->SafeAppend({drawPos - camera->GetRight() * drawRadius - camera->GetUp() * drawRadius, gfxt->xstart, gfxt->ystart, color});
	va->SafeAppend({drawPos + camera->GetRight() * drawRadius - camera->GetUp() * drawRadius, gfxt->xend,   gfxt->ystart, color});
	va->SafeAppend({drawPos + camera->GetRight() * drawRadius + camera->GetUp() * drawRadius, gfxt->xend,   gfxt->yend,   color});

	va->SafeAppend({drawPos + camera->GetRight() * drawRadius + camera->GetUp() * drawRadius, gfxt->xend,   gfxt->yend,   color});
	va->SafeAppend({drawPos - camera->GetRight() * drawRadius + camera->GetUp() * drawRadius, gfxt->xstart, gfxt->yend,   color});
	va->SafeAppend({drawPos - camera->GetRight() * drawRadius - camera->GetUp() * drawRadius, gfxt->xstart, gfxt->ystart, color});
}

void CNanoProjectile::DrawOnMinimap(GL::RenderDataBufferC* va)
{
	va->SafeAppend({pos        , color});
	va->SafeAppend({pos + speed, color});
}


bool CNanoProjectile::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_INT   (CNanoProjectile, deathFrame)
	CHECK_MEMBER_INFO_SCOLOR(CNanoProjectile, color     )

	return false;
}
