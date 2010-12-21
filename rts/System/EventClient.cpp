/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "EventClient.h"

/******************************************************************************/
/******************************************************************************/

CEventClient::CEventClient(const std::string& _name, int _order, bool _synced)
	: name(_name)
	, order(_order)
	, synced(_synced)
{
}


CEventClient::~CEventClient()
{
}


/******************************************************************************/
/******************************************************************************/
//
//  Synced
//

void CEventClient::Load(CArchiveBase* archive) {}
void CEventClient::GamePreload() {}
void CEventClient::GameStart() {}
void CEventClient::GameOver(std::vector<unsigned char> winningAllyTeams) {}
void CEventClient::GamePaused(int playerID, bool paused) {}
void CEventClient::GameFrame(int gameFrame) {}
void CEventClient::TeamDied(int teamID) {}
void CEventClient::TeamChanged(int teamID) {}
void CEventClient::PlayerChanged(int playerID) {}
void CEventClient::PlayerAdded(int playerID) {}
void CEventClient::PlayerRemoved(int playerID, int reason) {}

void CEventClient::UnitCreated(const CUnit* unit, const CUnit* builder) {}
void CEventClient::UnitFinished(const CUnit* unit) {}
void CEventClient::UnitFromFactory(const CUnit* unit, const CUnit* factory,
                                   bool userOrders) {}
void CEventClient::UnitDestroyed(const CUnit* unit, const CUnit* attacker) {}
void CEventClient::UnitTaken(const CUnit* unit, int newTeam) {}
void CEventClient::UnitGiven(const CUnit* unit, int oldTeam) {}

void CEventClient::UnitIdle(const CUnit* unit) {}
void CEventClient::UnitCommand(const CUnit* unit, const Command& command) {}
void CEventClient::UnitCmdDone(const CUnit* unit, int cmdType, int cmdTag) {}
void CEventClient::UnitDamaged(const CUnit* unit, const CUnit* attacker,
                               float damage, int weaponID, bool paralyzer) {}
void CEventClient::UnitExperience(const CUnit* unit, float oldExperience) {}

void CEventClient::UnitSeismicPing(const CUnit* unit, int allyTeam,
                             const float3& pos, float strength) {}
void CEventClient::UnitEnteredRadar(const CUnit* unit, int allyTeam) {}
void CEventClient::UnitEnteredLos(const CUnit* unit, int allyTeam) {}
void CEventClient::UnitLeftRadar(const CUnit* unit, int allyTeam) {}
void CEventClient::UnitLeftLos(const CUnit* unit, int allyTeam) {}

void CEventClient::UnitEnteredWater(const CUnit* unit) {}
void CEventClient::UnitEnteredAir(const CUnit* unit) {}
void CEventClient::UnitLeftWater(const CUnit* unit) {}
void CEventClient::UnitLeftAir(const CUnit* unit) {}

void CEventClient::UnitLoaded(const CUnit* unit, const CUnit* transport) {}
void CEventClient::UnitUnloaded(const CUnit* unit, const CUnit* transport) {}

void CEventClient::UnitCloaked(const CUnit* unit) {}
void CEventClient::UnitDecloaked(const CUnit* unit) {}

void CEventClient::RenderUnitCreated(const CUnit* unit, int cloaked) {}
void CEventClient::RenderUnitDestroyed(const CUnit* unit) {}
void CEventClient::RenderUnitCloakChanged(const CUnit* unit, int cloaked) {}
void CEventClient::RenderUnitLOSChanged(const CUnit* unit, int allyTeam, int newStatus) {}

void CEventClient::UnitUnitCollision(const CUnit* collider, const CUnit* collidee) {}
void CEventClient::UnitFeatureCollision(const CUnit* collider, const CFeature* collidee) {}
void CEventClient::UnitMoveFailed(const CUnit* unit) {}

void CEventClient::FeatureCreated(const CFeature* feature) {}
void CEventClient::FeatureDestroyed(const CFeature* feature) {}
void CEventClient::FeatureMoved(const CFeature* feature) {}

void CEventClient::RenderFeatureCreated(const CFeature* feature) {}
void CEventClient::RenderFeatureDestroyed(const CFeature* feature) {}
void CEventClient::RenderFeatureMoved(const CFeature* feature) {}

void CEventClient::ProjectileCreated(const CProjectile* proj) {}
void CEventClient::ProjectileDestroyed(const CProjectile* proj) {}

void CEventClient::RenderProjectileCreated(const CProjectile* proj) {}
void CEventClient::RenderProjectileDestroyed(const CProjectile* proj) {}

void CEventClient::StockpileChanged(const CUnit* unit,
                                    const CWeapon* weapon, int oldCount) {}

bool CEventClient::Explosion(int weaponID, const float3& pos, const CUnit* owner) { return false; }


/******************************************************************************/
/******************************************************************************/
//
//  Unsynced
//

void CEventClient::Save(zipFile archive) {}

void CEventClient::Update() {}

void CEventClient::ViewResize() {}

bool CEventClient::DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd) { return false; }

void CEventClient::DrawGenesis() {}
void CEventClient::DrawWorld() {}
void CEventClient::DrawWorldPreUnit() {}
void CEventClient::DrawWorldShadow() {}
void CEventClient::DrawWorldReflection() {}
void CEventClient::DrawWorldRefraction() {}
void CEventClient::DrawScreenEffects() {}
void CEventClient::DrawScreen() {}
void CEventClient::DrawInMiniMap() {}

// from LuaUI
bool CEventClient::KeyPress(unsigned short key, bool isRepeat) { return false; }
bool CEventClient::KeyRelease(unsigned short key) { return false; }
bool CEventClient::MouseMove(int x, int y, int dx, int dy, int button) { return false; }
bool CEventClient::MousePress(int x, int y, int button) { return false; }
int  CEventClient::MouseRelease(int x, int y, int button) { return -1; } // FIXME - bool / void?
bool CEventClient::MouseWheel(bool up, float value) { return false; }
bool CEventClient::JoystickEvent(const std::string& event, int val1, int val2) { return false; }
bool CEventClient::IsAbove(int x, int y) { return false; }
std::string CEventClient::GetTooltip(int x, int y) { return ""; }

bool CEventClient::CommandNotify(const Command& cmd) { return false; }

bool CEventClient::AddConsoleLine(const std::string& msg, const CLogSubsystem& subsystem) { return false; }

bool CEventClient::GroupChanged(int groupID) { return false; }

bool CEventClient::GameSetup(const std::string& state, bool& ready,
                             const map<int, std::string>& playerStates) { return false; }

std::string CEventClient::WorldTooltip(const CUnit* unit,
                                 const CFeature* feature,
                                 const float3* groundPos) { return ""; }

bool CEventClient::MapDrawCmd(int playerID, int type,
                        const float3* pos0,
                        const float3* pos1,
                        const std::string* label) { return false; }

/******************************************************************************/
/******************************************************************************/
