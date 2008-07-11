#include "StdAfx.h"
// EventClient.cpp: implementation of the CEventClient class.
//
//////////////////////////////////////////////////////////////////////

#include "EventClient.h"
using std::string;

/******************************************************************************/
/******************************************************************************/

CEventClient::CEventClient(const string& _name, int _order, bool _synced)
: name(_name),
  order(_order),
  synced(_synced)
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

void CEventClient::GamePreload() { return; }
void CEventClient::GameStart() { return; }
void CEventClient::GameOver() { return; }
void CEventClient::TeamDied(int teamID) { return; }
void CEventClient::TeamChanged(int teamID) { return; }
void CEventClient::PlayerChanged(int playerID) { return; }
void CEventClient::PlayerRemoved(int playerID) { return; }

void CEventClient::UnitCreated(const CUnit* unit, const CUnit* builder) { return; }
void CEventClient::UnitFinished(const CUnit* unit) { return; }
void CEventClient::UnitFromFactory(const CUnit* unit, const CUnit* factory,
                                   bool userOrders) { return; }
void CEventClient::UnitDestroyed(const CUnit* unit, const CUnit* attacker) { return; }
void CEventClient::UnitTaken(const CUnit* unit, int newTeam) { return; }
void CEventClient::UnitGiven(const CUnit* unit, int oldTeam) { return; }

void CEventClient::UnitIdle(const CUnit* unit) { return; }
void CEventClient::UnitCommand(const CUnit* unit, const Command& command) { return; }
void CEventClient::UnitCmdDone(const CUnit* unit, int cmdType, int cmdTag) { return; }
void CEventClient::UnitDamaged(const CUnit* unit, const CUnit* attacker,
                               float damage, int weaponID, bool paralyzer) { return; }
void CEventClient::UnitExperience(const CUnit* unit, float oldExperience) { return; }

void CEventClient::UnitSeismicPing(const CUnit* unit, int allyTeam,
                             const float3& pos, float strength) { return; }
void CEventClient::UnitEnteredRadar(const CUnit* unit, int allyTeam) { return; }
void CEventClient::UnitEnteredLos(const CUnit* unit, int allyTeam) { return; }
void CEventClient::UnitLeftRadar(const CUnit* unit, int allyTeam) { return; }
void CEventClient::UnitLeftLos(const CUnit* unit, int allyTeam) { return; }

void CEventClient::UnitEnteredWater(const CUnit* unit) { return; }
void CEventClient::UnitEnteredAir(const CUnit* unit) { return; }
void CEventClient::UnitLeftWater(const CUnit* unit) { return; }
void CEventClient::UnitLeftAir(const CUnit* unit) { return; }

void CEventClient::UnitLoaded(const CUnit* unit, const CUnit* transport) { return; }
void CEventClient::UnitUnloaded(const CUnit* unit, const CUnit* transport) { return; }

void CEventClient::UnitCloaked(const CUnit* unit) { return; }
void CEventClient::UnitDecloaked(const CUnit* unit) { return; }

void CEventClient::FeatureCreated(const CFeature* feature) { return; }
void CEventClient::FeatureDestroyed(const CFeature* feature) { return; }

void CEventClient::ProjectileCreated(const CProjectile* proj) { return; }
void CEventClient::ProjectileDestroyed(const CProjectile* proj) { return; }

void CEventClient::StockpileChanged(const CUnit* unit,
                                    const CWeapon* weapon, int oldCount) { return; }

bool CEventClient::Explosion(int weaponID, const float3& pos, const CUnit* owner)
{
  return false;
}


/******************************************************************************/
/******************************************************************************/
//
//  Unsynced
//

void CEventClient::Update() { return; }

void CEventClient::ViewResize() { return; }

bool CEventClient::DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd)
{
  return false;
}

void CEventClient::DrawGenesis() { return; }
void CEventClient::DrawWorld() { return; }
void CEventClient::DrawWorldPreUnit() { return; }
void CEventClient::DrawWorldShadow() { return; }
void CEventClient::DrawWorldReflection() { return; }
void CEventClient::DrawWorldRefraction() { return; }
void CEventClient::DrawScreenEffects() { return; }
void CEventClient::DrawScreen() { return; }
void CEventClient::DrawInMiniMap() { return; }

// from LuaUI
bool CEventClient::KeyPress(unsigned short key, bool isRepeat) { return false; }
bool CEventClient::KeyRelease(unsigned short key) { return false; }
bool CEventClient::MouseMove(int x, int y, int dx, int dy, int button) { return false; }
bool CEventClient::MousePress(int x, int y, int button) { return false; }
int  CEventClient::MouseRelease(int x, int y, int button) { return -1; } // FIXME - bool / void?
bool CEventClient::MouseWheel(bool up, float value) { return false; }
bool CEventClient::IsAbove(int x, int y) { return false; }
string CEventClient::GetTooltip(int x, int y) { return ""; }

bool CEventClient::CommandNotify(const Command& cmd) { return false; }

bool CEventClient::AddConsoleLine(const string& msg, int zone) { return false; }

bool CEventClient::GroupChanged(int groupID) { return false; }

bool CEventClient::GameSetup(const string& state, bool& ready,
                             const map<int, string>& playerStates) { return false; }

string CEventClient::WorldTooltip(const CUnit* unit,
                                 const CFeature* feature,
                                 const float3* groundPos) { return false; }

bool CEventClient::MapDrawCmd(int playerID, int type,
                        const float3* pos0,
                        const float3* pos1,
                        const string* label) { return false; }

/******************************************************************************/
/******************************************************************************/
