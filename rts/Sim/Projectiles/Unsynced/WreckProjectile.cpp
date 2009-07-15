// WreckProjectile.cpp: implementation of the CWreckProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Colors.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "SmokeProjectile.h"
#include "WreckProjectile.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CWreckProjectile, CProjectile, (float3(0, 0,0 ), float3(0, 0, 0), 0, 0));

CR_REG_METADATA(CWreckProjectile,
	CR_RESERVED(8)
);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWreckProjectile::CWreckProjectile(float3 pos,float3 speed,float temperature,CUnit* owner GML_PARG_C):
	CProjectile(pos,speed,owner, false, false, false GML_PARG_P)
{
	checkCol = false;
	drawRadius = 2.0f;
}

CWreckProjectile::~CWreckProjectile()
{

}

void CWreckProjectile::Update()
{
	speed.y += gravity;
	speed.x *= 0.994f;
	speed.z *= 0.994f;

	if (speed.y > 0)
		speed.y *= 0.998f;

	pos += speed;

	if (!(gs->frameNum & (ph->particleSaturation < 0.5f? 1: 3))) {
		CSmokeProjectile* hp = new CSmokeProjectile(pos, ZeroVector, 50, 4, 0.3f, owner(), 0.5f);
		hp->size += 0.1f;
	}
	if (pos.y + 0.3f < ground->GetApproximateHeight(pos.x, pos.z))
		deleteMe = true;
}

void CWreckProjectile::Draw(void)
{
	inArray=true;
	unsigned char col[4];
	col[0]=(unsigned char) (0.15f*200);
	col[1]=(unsigned char) (0.1f*200);
	col[2]=(unsigned char) (0.05f*200);
	col[3]=200;

	va->AddVertexTC(drawPos-camera->right*drawRadius-camera->up*drawRadius,ph->wrecktex.xstart,ph->wrecktex.ystart,col);
	va->AddVertexTC(drawPos+camera->right*drawRadius-camera->up*drawRadius,ph->wrecktex.xend,ph->wrecktex.ystart,col);
	va->AddVertexTC(drawPos+camera->right*drawRadius+camera->up*drawRadius,ph->wrecktex.xend,ph->wrecktex.yend,col);
	va->AddVertexTC(drawPos-camera->right*drawRadius+camera->up*drawRadius,ph->wrecktex.xstart,ph->wrecktex.yend,col);
}

void CWreckProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	points.AddVertexQC(pos, color4::redA);
}
