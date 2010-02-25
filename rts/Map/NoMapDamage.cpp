/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "MapDamage.h"
#include "NoMapDamage.h"
#include "float3.h"

/* Do no deformation. (Maybe I should've left all this in the header?) */

CNoMapDamage::CNoMapDamage(void)
{
	/* TODO What should the ground readouts display for hardness? */

	disabled=true;
}

CNoMapDamage::~CNoMapDamage(void)
{
}

void CNoMapDamage::Explosion(const float3& pos, float strength,float radius)
{
}

void CNoMapDamage::RecalcArea(int x1, int x2, int y1, int y2)
{
}

void CNoMapDamage::Update(void)
{
}
