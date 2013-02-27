/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NoWeapon.h"

CR_BIND_DERIVED(CNoWeapon, CWeapon, (NULL));

CNoWeapon::CNoWeapon(CUnit *owner) : CWeapon(owner)
{
	weaponDef=0;
}


void CNoWeapon::Update()
{
}

bool CNoWeapon::TestTarget(const float3& pos, bool userTarget, const CUnit* unit) const
{
	return false;
}

void CNoWeapon::Init()
{
}

void CNoWeapon::FireImpl()
{
}

void CNoWeapon::SlowUpdate()
{
}

