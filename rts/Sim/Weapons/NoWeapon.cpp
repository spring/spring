#include "StdAfx.h"
#include "NoWeapon.h"
#include "mmgr.h"

CR_BIND_DERIVED(CNoWeapon, CWeapon, (NULL));

CNoWeapon::CNoWeapon(CUnit *owner) : CWeapon(owner)
{
	weaponDef=0;
}


CNoWeapon::~CNoWeapon(void)
{
}

void CNoWeapon::Update(void)
{
}

bool CNoWeapon::TryTarget(const float3& pos,bool userTarget,CUnit* unit)
{
	return false;
}

void CNoWeapon::Init(void)
{
}

void CNoWeapon::FireImpl()
{
}

void CNoWeapon::SlowUpdate()
{
}

