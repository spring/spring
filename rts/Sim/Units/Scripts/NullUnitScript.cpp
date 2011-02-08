/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NullUnitScript.h"
#include "LogOutput.h"


// keep one global copy so we don't need to allocate a lot of
// near empty objects for mods that use Lua unit scripts.
CNullUnitScript CNullUnitScript::value;


CNullUnitScript::CNullUnitScript()
	: CUnitScript(NULL, pieces)
{
}


void CNullUnitScript::ShowScriptError(const std::string& msg)
{
	logOutput.Print(msg);
	logOutput.Print("why are you using CNullUnitScript anyway?");
}


void CNullUnitScript::RawCall(int functionId) {}
void CNullUnitScript::Create() {}
void CNullUnitScript::Killed() {}
void CNullUnitScript::WindChanged(float heading, float speed) {}
void CNullUnitScript::ExtractionRateChanged(float speed) {}
void CNullUnitScript::RockUnit(const float3& rockDir) {}
void CNullUnitScript::HitByWeapon(const float3& hitDir, int weaponDefId, float& inout_damage) {}
void CNullUnitScript::SetSFXOccupy(int curTerrainType) {}
void CNullUnitScript::QueryLandingPads(std::vector<int>& out_pieces) {}
void CNullUnitScript::BeginTransport(const CUnit* unit) {}
int  CNullUnitScript::QueryTransport(const CUnit* unit) { return -1; }
void CNullUnitScript::TransportPickup(const CUnit* unit) {}
void CNullUnitScript::TransportDrop(const CUnit* unit, const float3& pos) {}
void CNullUnitScript::StartBuilding(float heading, float pitch) {}
int  CNullUnitScript::QueryNanoPiece() { return -1; }
int  CNullUnitScript::QueryBuildInfo() { return -1; }
int  CNullUnitScript::QueryWeapon(int weaponNum) { return -1; }
void CNullUnitScript::AimWeapon(int weaponNum, float heading, float pitch) {}
void CNullUnitScript::AimShieldWeapon(CPlasmaRepulser* weapon) {}
int  CNullUnitScript::AimFromWeapon(int weaponNum) { return -1; }
void CNullUnitScript::Shot(int weaponNum) {}
bool CNullUnitScript::BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget) { return false; }
float CNullUnitScript::TargetWeight(int weaponNum, const CUnit* targetUnit) { return 1.0f; }

void CNullUnitScript::Destroy()       {}
void CNullUnitScript::StartMoving()   {}
void CNullUnitScript::StopMoving()    {}
void CNullUnitScript::StartUnload()   {}
void CNullUnitScript::EndTransport()  {}
void CNullUnitScript::StartBuilding() {}
void CNullUnitScript::StopBuilding()  {}
void CNullUnitScript::Falling()       {}
void CNullUnitScript::Landed()        {}
void CNullUnitScript::Activate()      {}
void CNullUnitScript::Deactivate()    {}
void CNullUnitScript::MoveRate(int curRate)     {}
void CNullUnitScript::FireWeapon(int weaponNum) {}
void CNullUnitScript::EndBurst(int weaponNum)   {}
