#include "System/StdAfx.h"
#include "NoWeapon.h"
//#include "System/mmgr.h"

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

void CNoWeapon::Fire(void)
{

}

void CNoWeapon::SlowUpdate(void)
{

}

