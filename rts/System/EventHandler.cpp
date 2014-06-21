/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/EventHandler.h"
#include "System/EventBatchHandler.h"

#include "Lua/LuaCallInCheck.h"
#include "Lua/LuaOpenGL.h"  // FIXME -- should be moved

#include "System/Config/ConfigHandler.h"
#include "System/Platform/Threading.h"
#include "System/GlobalConfig.h"

using std::string;
using std::vector;
using std::map;


CEventHandler eventHandler;


/******************************************************************************/
/******************************************************************************/

void CEventHandler::SetupEvent(const string& eName,
                               EventClientList* list, int props)
{
	eventMap[eName] = EventInfo(eName, list, props);
}

/******************************************************************************/
/******************************************************************************/

CEventHandler::CEventHandler()
{
	mouseOwner = NULL;

	// Setup all events
	#define SETUP_EVENT(name, props) SetupEvent(#name, &list ## name, props)
	#define SETUP_UNMANAGED_EVENT(name, props) SetupEvent(#name, NULL, props)
		#include "Events.def"
	#undef SETUP_EVENT
	#undef SETUP_UNMANAGED_EVENT
}


CEventHandler::~CEventHandler()
{
}


/******************************************************************************/
/******************************************************************************/

void CEventHandler::AddClient(CEventClient* ec)
{
	ListInsert(handles, ec);

	EventMap::const_iterator it;
	for (it = eventMap.begin(); it != eventMap.end(); ++it) {
		const EventInfo& ei = it->second;
		if (ei.HasPropBit(MANAGED_BIT) && (ei.GetList() != NULL)) {
			if (ec->WantsEvent(it->first)) {
				ListInsert(*ei.GetList(), ec);
			}
		}
	}
}


void CEventHandler::RemoveClient(CEventClient* ec)
{
	if (mouseOwner == ec) {
		mouseOwner = NULL;
	}

	ListRemove(handles, ec);

	EventMap::const_iterator it;
	for (it = eventMap.begin(); it != eventMap.end(); ++it) {
		const EventInfo& ei = it->second;
		if (ei.HasPropBit(MANAGED_BIT) && (ei.GetList() != NULL)) {
			ListRemove(*ei.GetList(), ec);
		}
	}
}


/******************************************************************************/
/******************************************************************************/

void CEventHandler::GetEventList(vector<string>& list) const
{
	list.clear();
	EventMap::const_iterator it;
	for (it = eventMap.begin(); it != eventMap.end(); ++it) {
		list.push_back(it->first);
	}
}


bool CEventHandler::IsKnown(const string& eName) const
{
	return (eventMap.find(eName) != eventMap.end());
}


bool CEventHandler::IsManaged(const string& eName) const
{
	EventMap::const_iterator it = eventMap.find(eName);
	return ((it != eventMap.end()) && (it->second.HasPropBit(MANAGED_BIT)));
}


bool CEventHandler::IsUnsynced(const string& eName) const
{
	EventMap::const_iterator it = eventMap.find(eName);
	return ((it != eventMap.end()) && (it->second.HasPropBit(UNSYNCED_BIT)));
}


bool CEventHandler::IsController(const string& eName) const
{
	EventMap::const_iterator it = eventMap.find(eName);
	return ((it != eventMap.end()) && (it->second.HasPropBit(CONTROL_BIT)));
}


/******************************************************************************/

bool CEventHandler::InsertEvent(CEventClient* ec, const string& ciName)
{
	EventMap::iterator it = eventMap.find(ciName);
	if ((it == eventMap.end()) || (it->second.GetList() == NULL)) {
		return false;
	}
	if (ec->GetSynced() && it->second.HasPropBit(UNSYNCED_BIT)) {
		return false;
	}
	ListInsert(*it->second.GetList(), ec);
	return true;
}


bool CEventHandler::RemoveEvent(CEventClient* ec, const string& ciName)
{
	EventMap::iterator it = eventMap.find(ciName);
	if ((it == eventMap.end()) || (it->second.GetList() == NULL)) {
		return false;
	}
	ListRemove(*it->second.GetList(), ec);
	return true;
}


/******************************************************************************/

void CEventHandler::ListInsert(EventClientList& ecList, CEventClient* ec)
{
	EventClientList::iterator it;
	for (it = ecList.begin(); it != ecList.end(); ++it) {
		const CEventClient* ecIt = *it;
		if (ec == ecIt) {
			return; // already in the list
		}
		else if ((ec->GetOrder()  <  ecIt->GetOrder()) ||
		         ((ec->GetOrder() == ecIt->GetOrder()) &&
		          (ec->GetName()  <  ecIt->GetName()))) { // should not happen
			ecList.insert(it, ec);
			return;
		}
	}
	ecList.push_back(ec);
}


void CEventHandler::ListRemove(EventClientList& ecList, CEventClient* ec)
{
	// FIXME: efficient, hardly
	EventClientList newList;
	newList.reserve(ecList.size());
	for (size_t i = 0; i < ecList.size(); i++) {
		if (ec != ecList[i]) {
			newList.push_back(ecList[i]);
		}
	}
	ecList.swap(newList);
}


/******************************************************************************/
/******************************************************************************/

#define CONTROL_ITERATE_DEF_TRUE(name, ...) \
	bool result = true;                        \
	for (int i = 0; i < list##name.size(); ) { \
		CEventClient* ec = list##name[i];  \
		result &= ec->name(__VA_ARGS__);   \
		if (i < list##name.size() && ec == list##name[i]) \
			++i; /* the call-in may remove itself from the list */ \
	} \
	return result;

#define CONTROL_ITERATE_DEF_FALSE(name, ...) \
	bool result = false;                        \
	for (int i = 0; i < list##name.size(); ) { \
		CEventClient* ec = list##name[i];  \
		result |= ec->name(__VA_ARGS__);   \
		if (i < list##name.size() && ec == list##name[i]) \
			++i; /* the call-in may remove itself from the list */ \
	} \
	return result;


bool CEventHandler::CommandFallback(const CUnit* unit, const Command& cmd)
{
	CONTROL_ITERATE_DEF_TRUE(CommandFallback, unit, cmd)
}


bool CEventHandler::AllowCommand(const CUnit* unit, const Command& cmd, bool fromSynced)
{
	CONTROL_ITERATE_DEF_TRUE(AllowCommand, unit, cmd, fromSynced)
}


bool CEventHandler::AllowUnitCreation(const UnitDef* unitDef, const CUnit* builder, const BuildInfo* buildInfo)
{
	CONTROL_ITERATE_DEF_TRUE(AllowUnitCreation, unitDef, builder, buildInfo)
}


bool CEventHandler::AllowUnitTransfer(const CUnit* unit, int newTeam, bool capture)
{
	CONTROL_ITERATE_DEF_TRUE(AllowUnitTransfer, unit, newTeam, capture)
}


bool CEventHandler::AllowUnitBuildStep(const CUnit* builder, const CUnit* unit, float part)
{
	CONTROL_ITERATE_DEF_TRUE(AllowUnitBuildStep, builder, unit, part)
}


bool CEventHandler::AllowFeatureCreation(const FeatureDef* featureDef, int allyTeamID, const float3& pos)
{
	CONTROL_ITERATE_DEF_TRUE(AllowFeatureCreation, featureDef, allyTeamID, pos)
}


bool CEventHandler::AllowFeatureBuildStep(const CUnit* builder, const CFeature* feature, float part)
{
	CONTROL_ITERATE_DEF_TRUE(AllowFeatureBuildStep, builder, feature, part)
}


bool CEventHandler::AllowResourceLevel(int teamID, const string& type, float level)
{
	CONTROL_ITERATE_DEF_TRUE(AllowResourceLevel, teamID, type, level)
}


bool CEventHandler::AllowResourceTransfer(int oldTeam, int newTeam, const string& type, float amount)
{
	CONTROL_ITERATE_DEF_TRUE(AllowResourceTransfer, oldTeam, newTeam, type, amount)
}


bool CEventHandler::AllowDirectUnitControl(int playerID, const CUnit* unit)
{
	CONTROL_ITERATE_DEF_TRUE(AllowDirectUnitControl, playerID, unit)
}


bool CEventHandler::AllowBuilderHoldFire(const CUnit* unit, int action)
{
	CONTROL_ITERATE_DEF_TRUE(AllowBuilderHoldFire, unit, action)
}


bool CEventHandler::AllowStartPosition(int playerID, unsigned char readyState, const float3& clampedPos, const float3& rawPickPos)
{
	CONTROL_ITERATE_DEF_TRUE(AllowStartPosition, playerID, readyState, clampedPos, rawPickPos)
}



bool CEventHandler::TerraformComplete(const CUnit* unit, const CUnit* build)
{
	CONTROL_ITERATE_DEF_FALSE(TerraformComplete, unit, build)
}


bool CEventHandler::MoveCtrlNotify(const CUnit* unit, int data)
{
	CONTROL_ITERATE_DEF_FALSE(MoveCtrlNotify, unit, data)
}


int CEventHandler::AllowWeaponTargetCheck(unsigned int attackerID, unsigned int attackerWeaponNum, unsigned int attackerWeaponDefID)
{
	int result = -1;
	for (int i = 0; i < listAllowWeaponTargetCheck.size(); ) {
		CEventClient* ec = listAllowWeaponTargetCheck[i];
		int result2 = ec->AllowWeaponTargetCheck(attackerID, attackerWeaponNum, attackerWeaponDefID);
		if (result2 > result) result = result2;
		if (i < listAllowWeaponTargetCheck.size() && ec == listAllowWeaponTargetCheck[i])
			++i; /* the call-in may remove itself from the list */
	}
	return result;
}


bool CEventHandler::AllowWeaponTarget(
	unsigned int attackerID,
	unsigned int targetID,
	unsigned int attackerWeaponNum,
	unsigned int attackerWeaponDefID,
	float* targetPriority
) {
	CONTROL_ITERATE_DEF_TRUE(AllowWeaponTarget, attackerID, targetID, attackerWeaponNum, attackerWeaponDefID, targetPriority)
}


bool CEventHandler::AllowWeaponInterceptTarget(const CUnit* interceptorUnit, const CWeapon* interceptorWeapon, const CProjectile* interceptorTarget)
{
	CONTROL_ITERATE_DEF_TRUE(AllowWeaponInterceptTarget, interceptorUnit, interceptorWeapon, interceptorTarget)
}


bool CEventHandler::UnitPreDamaged(
	const CUnit* unit,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID,
	bool paralyzer,
	float* newDamage,
	float* impulseMult
) {
	CONTROL_ITERATE_DEF_FALSE(UnitPreDamaged, unit, attacker, damage, weaponDefID, projectileID, paralyzer, newDamage, impulseMult)
}


bool CEventHandler::FeaturePreDamaged(
	const CFeature* feature,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID,
	float* newDamage,
	float* impulseMult
) {
	CONTROL_ITERATE_DEF_FALSE(FeaturePreDamaged, feature, attacker, damage, weaponDefID, projectileID, newDamage, impulseMult)
}


bool CEventHandler::ShieldPreDamaged(const CProjectile* proj, const CWeapon* w, const CUnit* u, bool repulsor)
{
	CONTROL_ITERATE_DEF_FALSE(ShieldPreDamaged, proj, w, u, repulsor)
}


bool CEventHandler::SyncedActionFallback(const string& line, int playerID)
{
	for (int i = 0; i < listSyncedActionFallback.size(); ) {
		CEventClient* ec = listSyncedActionFallback[i];
		if (ec->SyncedActionFallback(line, playerID))
			return true;
		if (i < listSyncedActionFallback.size() && ec == listSyncedActionFallback[i])
			++i; /* the call-in may remove itself from the list */
	}
	return false;
}


/******************************************************************************/
/******************************************************************************/

#define ITERATE_EVENTCLIENTLIST(name, ...) \
	for (int i = 0; i < list##name.size(); ) { \
		CEventClient* ec = list##name[i]; \
		ec->name(__VA_ARGS__); \
		if (i < list##name.size() && ec == list##name[i]) \
			++i; /* the call-in may remove itself from the list */ \
	}


void CEventHandler::Save(zipFile archive)
{
	ITERATE_EVENTCLIENTLIST(Save, archive);
}


void CEventHandler::GamePreload()
{
	ITERATE_EVENTCLIENTLIST(GamePreload);
}


void CEventHandler::GameStart()
{
	ITERATE_EVENTCLIENTLIST(GameStart);
}


void CEventHandler::GameOver(const std::vector<unsigned char>& winningAllyTeams)
{
	ITERATE_EVENTCLIENTLIST(GameOver, winningAllyTeams);
}


void CEventHandler::GamePaused(int playerID, bool paused)
{
	ITERATE_EVENTCLIENTLIST(GamePaused, playerID, paused);
}


void CEventHandler::GameFrame(int gameFrame)
{
	ITERATE_EVENTCLIENTLIST(GameFrame, gameFrame);
}


void CEventHandler::GameID(const unsigned char* gameID, unsigned int numBytes)
{
	ITERATE_EVENTCLIENTLIST(GameID, gameID, numBytes);
}


void CEventHandler::TeamDied(int teamID)
{
	ITERATE_EVENTCLIENTLIST(TeamDied, teamID);
}


void CEventHandler::TeamChanged(int teamID)
{
	ITERATE_EVENTCLIENTLIST(TeamChanged, teamID);
}


void CEventHandler::PlayerChanged(int playerID)
{
	ITERATE_EVENTCLIENTLIST(PlayerChanged, playerID);
}


void CEventHandler::PlayerAdded(int playerID)
{
	ITERATE_EVENTCLIENTLIST(PlayerAdded, playerID);
}


void CEventHandler::PlayerRemoved(int playerID, int reason)
{
	ITERATE_EVENTCLIENTLIST(PlayerRemoved, playerID, reason);
}


/******************************************************************************/
/******************************************************************************/

void CEventHandler::UnitHarvestStorageFull(const CUnit* unit)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitHarvestStorageFull.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listUnitHarvestStorageFull[i];
		if (ec->CanReadAllyTeam(unitAllyTeam)) {
			ec->UnitHarvestStorageFull(unit);
		}
	}
}

/******************************************************************************/
/******************************************************************************/

void CEventHandler::CollectGarbage()
{
	ITERATE_EVENTCLIENTLIST(CollectGarbage);
}


void CEventHandler::DbgTimingInfo(DbgTimingInfoType type, const spring_time start, const spring_time end)
{
	ITERATE_EVENTCLIENTLIST(DbgTimingInfo, type, start, end);
}


void CEventHandler::Load(IArchive* archive)
{
	ITERATE_EVENTCLIENTLIST(Load, archive);
}


void CEventHandler::Update()
{
	ITERATE_EVENTCLIENTLIST(Update);
}



void CEventHandler::UpdateUnits() { eventBatchHandler->UpdateUnits(); }
void CEventHandler::UpdateDrawUnits() { eventBatchHandler->UpdateDrawUnits(); }
void CEventHandler::DeleteSyncedUnits() { eventBatchHandler->DeleteSyncedUnits(); }

void CEventHandler::UpdateFeatures() { eventBatchHandler->UpdateFeatures(); }
void CEventHandler::UpdateDrawFeatures() { eventBatchHandler->UpdateDrawFeatures(); }
void CEventHandler::DeleteSyncedFeatures() { eventBatchHandler->DeleteSyncedFeatures(); }

void CEventHandler::UpdateProjectiles() { eventBatchHandler->UpdateProjectiles(); }
void CEventHandler::UpdateDrawProjectiles() { eventBatchHandler->UpdateDrawProjectiles(); }

inline void ExecuteAllCallsFromSynced() { } //FIXME delete

void CEventHandler::DeleteSyncedProjectiles() {
	ExecuteAllCallsFromSynced();
	eventBatchHandler->DeleteSyncedProjectiles();
}

void CEventHandler::UpdateObjects() {
	eventBatchHandler->UpdateObjects();
}
void CEventHandler::DeleteSyncedObjects() {
	ExecuteAllCallsFromSynced();
}



void CEventHandler::SunChanged(const float3& sunDir)
{
	ITERATE_EVENTCLIENTLIST(SunChanged, sunDir);
}

void CEventHandler::ViewResize()
{
	ITERATE_EVENTCLIENTLIST(ViewResize);
}


void CEventHandler::GameProgress(int gameFrame)
{
	ITERATE_EVENTCLIENTLIST(GameProgress, gameFrame);
}


#define DRAW_CALLIN(name)                         \
  void CEventHandler:: Draw ## name ()            \
  {                                               \
    if (listDraw ## name.empty())                 \
      return;                                     \
    LuaOpenGL::EnableDraw ## name ();             \
    listDraw ## name [0]->Draw ## name ();        \
                                                  \
    for (int i = 1; i < listDraw ## name.size(); ) { \
      LuaOpenGL::ResetDraw ## name ();            \
      CEventClient* ec = listDraw ## name [i];    \
      ec-> Draw ## name ();                       \
      if (i < listDraw ## name.size() && ec == listDraw ## name [i]) \
	    ++i;                                      \
    }                                             \
                                                  \
    LuaOpenGL::DisableDraw ## name ();            \
  }

DRAW_CALLIN(Genesis)
DRAW_CALLIN(World)
DRAW_CALLIN(WorldPreUnit)
DRAW_CALLIN(WorldShadow)
DRAW_CALLIN(WorldReflection)
DRAW_CALLIN(WorldRefraction)
DRAW_CALLIN(ScreenEffects)
DRAW_CALLIN(Screen)
DRAW_CALLIN(InMiniMap)


#define DRAW_ENTITY_CALLIN(name, args, args2)     \
  bool CEventHandler:: Draw ## name args        \
  {                                               \
    bool skipEngineDrawing = false;               \
    for (int i = 0; i < listDraw ## name.size(); ) { \
      CEventClient* ec = listDraw ## name [i];    \
      skipEngineDrawing |= ec-> Draw ## name args2 ; \
      if (i < listDraw ## name.size() && ec == listDraw ## name [i]) \
	    ++i;                                      \
    } \
    return skipEngineDrawing; \
  }

DRAW_ENTITY_CALLIN(Unit, (const CUnit* unit), (unit))
DRAW_ENTITY_CALLIN(Feature, (const CFeature* feature), (feature))
DRAW_ENTITY_CALLIN(Shield, (const CUnit* unit, const CWeapon* weapon), (unit, weapon))
DRAW_ENTITY_CALLIN(Projectile, (const CProjectile* projectile), (projectile))

/******************************************************************************/
/******************************************************************************/

#define CONTROL_REVERSE_ITERATE_DEF_TRUE(name, ...) \
	for (int i = list##name.size() - 1; i >= 0; --i) { \
		CEventClient* ec = list##name[i]; \
		if (ec->name(__VA_ARGS__)) \
			return true; \
	}

#define CONTROL_REVERSE_ITERATE_STRING(name, ...) \
	for (int i = list##name.size() - 1; i >= 0; --i) { \
		CEventClient* ec = list##name[i]; \
		const std::string& str = ec->name(__VA_ARGS__); \
		if (!str.empty()) \
			return str; \
	}

bool CEventHandler::CommandNotify(const Command& cmd)
{
	CONTROL_REVERSE_ITERATE_DEF_TRUE(CommandNotify, cmd)
	return false;
}


bool CEventHandler::KeyPress(int key, bool isRepeat)
{
	CONTROL_REVERSE_ITERATE_DEF_TRUE(KeyPress, key, isRepeat)
	return false;
}


bool CEventHandler::KeyRelease(int key)
{
	CONTROL_REVERSE_ITERATE_DEF_TRUE(KeyRelease, key)
	return false;
}


bool CEventHandler::TextInput(const std::string& utf8)
{
	CONTROL_REVERSE_ITERATE_DEF_TRUE(TextInput, utf8)
	return false;
}


bool CEventHandler::MousePress(int x, int y, int button)
{
	for (int i = listMousePress.size() - 1; i >= 0; --i) {
		CEventClient* ec = listMousePress[i];
		if (ec->MousePress(x,y,button)) {
			if (!mouseOwner)
				mouseOwner = ec;
			return true;
		}
	}
	return false;
}


void CEventHandler::MouseRelease(int x, int y, int button)
{
	if (mouseOwner) {
		mouseOwner->MouseRelease(x, y, button);
		mouseOwner = NULL;
	}
}


bool CEventHandler::MouseMove(int x, int y, int dx, int dy, int button)
{
	if (mouseOwner == NULL) {
		return false;
	}

	return mouseOwner->MouseMove(x, y, dx, dy, button);
}


bool CEventHandler::MouseWheel(bool up, float value)
{
	CONTROL_REVERSE_ITERATE_DEF_TRUE(MouseWheel, up, value)
	return false;
}

bool CEventHandler::JoystickEvent(const std::string& event, int val1, int val2)
{
	CONTROL_REVERSE_ITERATE_DEF_TRUE(JoystickEvent, event, val1, val2)
	return false;
}

bool CEventHandler::IsAbove(int x, int y)
{
	CONTROL_REVERSE_ITERATE_DEF_TRUE(IsAbove, x, y)
	return false;
}

string CEventHandler::GetTooltip(int x, int y)
{
	CONTROL_REVERSE_ITERATE_STRING(GetTooltip, x, y)
	return "";
}


bool CEventHandler::AddConsoleLine(const std::string& msg, const std::string& section, int level)
{
	if (listAddConsoleLine.empty())
		return false;

	ITERATE_EVENTCLIENTLIST(AddConsoleLine, msg, section, level);
	return true;
}


void CEventHandler::LastMessagePosition(const float3& pos)
{
	ITERATE_EVENTCLIENTLIST(LastMessagePosition, pos);
}


bool CEventHandler::GroupChanged(int groupID)
{
	ITERATE_EVENTCLIENTLIST(GroupChanged, groupID);
	return false;
}



bool CEventHandler::GameSetup(const string& state, bool& ready,
                                  const map<int, string>& playerStates)
{
	CONTROL_REVERSE_ITERATE_DEF_TRUE(GameSetup, state, ready, playerStates)
	return false;
}


string CEventHandler::WorldTooltip(const CUnit* unit,
                                   const CFeature* feature,
                                   const float3* groundPos)
{
	CONTROL_REVERSE_ITERATE_STRING(WorldTooltip, unit, feature, groundPos)
	return "";
}


bool CEventHandler::MapDrawCmd(int playerID, int type,
                               const float3* pos0, const float3* pos1,
                                   const string* label)
{
	CONTROL_REVERSE_ITERATE_DEF_TRUE(MapDrawCmd, playerID, type, pos0, pos1, label)
	return false;
}


/******************************************************************************/
/******************************************************************************/

void CEventHandler::UnsyncedProjectileCreated(const CProjectile* proj) {
	//FIXME no real event
	(eventBatchHandler->GetUnsyncedProjectileCreatedDestroyedBatch()).insert(proj);
}

void CEventHandler::UnsyncedProjectileDestroyed(const CProjectile* proj) {
	//FIXME no real event
	(eventBatchHandler->GetUnsyncedProjectileCreatedDestroyedBatch()).erase_delete(proj);
}

void CEventHandler::LoadedModelRequested() {
	//FIXME no real event
	eventBatchHandler->LoadedModelRequested();
}


/******************************************************************************/
/******************************************************************************/

