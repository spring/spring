/* Author: Tobi Vollebregt */

#include "NullUnitScript.h"
#include "LogOutput.h"


CNullUnitScript::CNullUnitScript(CUnit* unit)
	: CUnitScript(unit, scriptIndex, unit->localmodel->pieces)
	, scriptIndex(COBFN_Last + (unit->weapons.size() * COBFN_Weapon_Funcs), -1)
{
}


void CNullUnitScript::ShowScriptError(const std::string& msg)
{
	logOutput.Print(msg);
	logOutput.Print("why are you using CNullUnitScript anyway?");
}


void CNullUnitScript::RawCall(int functionId)
{
}


void CNullUnitScript::Create()
{
}


void CNullUnitScript::Killed()
{
}


void CNullUnitScript::SetDirection(float heading)
{
}


void CNullUnitScript::SetSpeed(float speed, float cob_mult)
{
}


void CNullUnitScript::RockUnit(const float3& rockDir)
{
}


void CNullUnitScript::HitByWeapon(const float3& hitDir)
{
}


void CNullUnitScript::HitByWeaponId(const float3& hitDir, int weaponDefId, float& inout_damage)
{
}


void CNullUnitScript::SetSFXOccupy(int curTerrainType)
{
}


void CNullUnitScript::QueryLandingPads(std::vector<int>& out_pieces)
{
}


void CNullUnitScript::BeginTransport(const CUnit* unit)
{
}


int CNullUnitScript::QueryTransport(const CUnit* unit)
{
	return -1;
}


void CNullUnitScript::TransportPickup(const CUnit* unit)
{
}


void CNullUnitScript::TransportDrop(const CUnit* unit, const float3& pos)
{
}


void CNullUnitScript::StartBuilding(float heading, float pitch)
{
}


int CNullUnitScript::QueryNanoPiece()
{
	return -1;
}


int CNullUnitScript::QueryBuildInfo()
{
	return -1;
}


int CNullUnitScript::QueryWeapon(int weaponNum)
{
	return -1;
}


void CNullUnitScript::AimWeapon(int weaponNum, float heading, float pitch)
{
}


void CNullUnitScript::AimShieldWeapon(CPlasmaRepulser* weapon)
{
}


int CNullUnitScript::AimFromWeapon(int weaponNum)
{
	return -1;
}


void CNullUnitScript::Shot(int weaponNum)
{
}


bool CNullUnitScript::BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget)
{
	return false;
}


float CNullUnitScript::TargetWeight(int weaponNum, const CUnit* targetUnit)
{
	return 1.0f;
}
