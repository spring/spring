/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "System/EventClient.h"
#include "System/EventHandler.h"

/******************************************************************************/
/******************************************************************************/

CEventClient::CEventClient(const std::string& _name, int _order, bool _synced)
	: name(_name)
	, order(_order)
	, synced_(_synced)
	, autoLinkEvents(false)
{
	// Note: virtual functions aren't available in the ctor!
	//RegisterLinkedEvents(this);
}


CEventClient::~CEventClient()
{
	// No, we can't autobind all clients in the ctor.
	// eventHandler.AddClient() calls CEventClient::WantsEvent() that is
	// virtual and so not available during the initialization.
	eventHandler.RemoveClient(this);
}


bool CEventClient::WantsEvent(const std::string& eventName)
{
	if (!autoLinkEvents)
		return false;

	assert(!autoLinkedEvents.empty());

	if (autoLinkedEvents[eventName]) {
		//LOG("\"%s\" autolinks \"%s\"", GetName().c_str(), eventName.c_str());
		return true;
	}

	return false;
}


/******************************************************************************/
/******************************************************************************/
//
//  Synced
//

bool CEventClient::CommandFallback(const CUnit* unit, const Command& cmd) { return false; }
bool CEventClient::AllowCommand(const CUnit* unit, const Command& cmd, bool fromSynced) { return true; }

bool CEventClient::AllowUnitCreation(const UnitDef* unitDef, const CUnit* builder, const BuildInfo* buildInfo) { return true; }
bool CEventClient::AllowUnitTransfer(const CUnit* unit, int newTeam, bool capture) { return true; }
bool CEventClient::AllowUnitBuildStep(const CUnit* builder, const CUnit* unit, float part) { return true; }
bool CEventClient::AllowFeatureCreation(const FeatureDef* featureDef, int allyTeamID, const float3& pos) { return true; }
bool CEventClient::AllowFeatureBuildStep(const CUnit* builder, const CFeature* feature, float part) { return true; }
bool CEventClient::AllowResourceLevel(int teamID, const string& type, float level) { return true; }
bool CEventClient::AllowResourceTransfer(int oldTeam, int newTeam, const string& type, float amount) { return true; }
bool CEventClient::AllowDirectUnitControl(int playerID, const CUnit* unit) { return true; }
bool CEventClient::AllowBuilderHoldFire(const CUnit* unit, int action) { return true; }
bool CEventClient::AllowStartPosition(int playerID, unsigned char readyState, const float3& clampedPos, const float3& rawPickPos) { return true; }

bool CEventClient::TerraformComplete(const CUnit* unit, const CUnit* build) { return false; }
bool CEventClient::MoveCtrlNotify(const CUnit* unit, int data) { return false; }

int CEventClient::AllowWeaponTargetCheck(unsigned int attackerID, unsigned int attackerWeaponNum, unsigned int attackerWeaponDefID) { return -1; }
bool CEventClient::AllowWeaponTarget(
	unsigned int attackerID,
	unsigned int targetID,
	unsigned int attackerWeaponNum,
	unsigned int attackerWeaponDefID,
	float* targetPriority
) { return true; }
bool CEventClient::AllowWeaponInterceptTarget(const CUnit* interceptorUnit, const CWeapon* interceptorWeapon, const CProjectile* interceptorTarget) { return true; }
bool CEventClient::UnitPreDamaged(
	const CUnit* unit,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID,
	bool paralyzer,
	float* newDamage,
	float* impulseMult) { return false; }
bool CEventClient::FeaturePreDamaged(
	const CFeature* feature,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID,
	float* newDamage,
	float* impulseMult) { return false; }
bool CEventClient::ShieldPreDamaged(const CProjectile*, const CWeapon*, const CUnit*, bool) { return false; }
bool CEventClient::SyncedActionFallback(const string& line, int playerID) { return false; }

/******************************************************************************/
/******************************************************************************/
//
//  Unsynced
//

void CEventClient::Save(zipFile archive) {}

void CEventClient::Update() {}
void CEventClient::UnsyncedHeightMapUpdate(const SRectangle& rect) {}

void CEventClient::SunChanged(const float3& sunDir) {}

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

bool CEventClient::DrawUnit(const CUnit* unit) { return false; }
bool CEventClient::DrawFeature(const CFeature* feature) { return false; }
bool CEventClient::DrawShield(const CUnit* unit, const CWeapon* weapon) { return false; }
bool CEventClient::DrawProjectile(const CProjectile* projectile) { return false; }

void CEventClient::GameProgress(int gameFrame) {}

void CEventClient::DrawLoadScreen() {}
void CEventClient::LoadProgress(const std::string& msg, const bool replace_lastline) {}

void CEventClient::CollectGarbage() {}
void CEventClient::DbgTimingInfo(DbgTimingInfoType type, const spring_time start, const spring_time end) {}

// from LuaUI
bool CEventClient::KeyPress(int key, bool isRepeat) { return false; }
bool CEventClient::KeyRelease(int key) { return false; }
bool CEventClient::TextInput(const std::string& utf8) { return false; }
bool CEventClient::MouseMove(int x, int y, int dx, int dy, int button) { return false; }
bool CEventClient::MousePress(int x, int y, int button) { return false; }
void CEventClient::MouseRelease(int x, int y, int button) { }
bool CEventClient::MouseWheel(bool up, float value) { return false; }
bool CEventClient::JoystickEvent(const std::string& event, int val1, int val2) { return false; }
bool CEventClient::IsAbove(int x, int y) { return false; }
std::string CEventClient::GetTooltip(int x, int y) { return ""; }

bool CEventClient::CommandNotify(const Command& cmd) { return false; }

bool CEventClient::AddConsoleLine(const std::string& msg, const std::string& section, int level) { return false; }

void CEventClient::LastMessagePosition(const float3& pos) {}

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
