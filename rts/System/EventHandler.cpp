/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/EventHandler.h"

#include "Lua/LuaCallInCheck.h"
#include "Lua/LuaOpenGL.h"  // FIXME -- should be moved
#include "Lua/LuaRules.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaUI.h"  // FIXME -- should be moved

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

#define SETUP_EVENT(name, props) SetupEvent(#name, &list ## name, props)

/******************************************************************************/
/******************************************************************************/

CEventHandler::CEventHandler()
{
	mouseOwner = NULL;

	// synced call-ins
	SETUP_EVENT(Load, MANAGED_BIT);

	SETUP_EVENT(GamePreload,   MANAGED_BIT);
	SETUP_EVENT(GameStart,     MANAGED_BIT);
	SETUP_EVENT(GameOver,      MANAGED_BIT);
	SETUP_EVENT(GamePaused,    MANAGED_BIT);
	SETUP_EVENT(GameFrame,     MANAGED_BIT);
	SETUP_EVENT(GameID,        MANAGED_BIT);

	SETUP_EVENT(TeamDied,      MANAGED_BIT);
	SETUP_EVENT(TeamChanged,   MANAGED_BIT);
	SETUP_EVENT(PlayerChanged, MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(PlayerAdded,   MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(PlayerRemoved, MANAGED_BIT | UNSYNCED_BIT);

	SETUP_EVENT(UnitCreated,     MANAGED_BIT);
	SETUP_EVENT(UnitFinished,    MANAGED_BIT);
	SETUP_EVENT(UnitFromFactory, MANAGED_BIT);
	SETUP_EVENT(UnitDestroyed,   MANAGED_BIT);
	SETUP_EVENT(UnitTaken,       MANAGED_BIT);
	SETUP_EVENT(UnitGiven,       MANAGED_BIT);

	SETUP_EVENT(UnitIdle,       MANAGED_BIT);
	SETUP_EVENT(UnitCommand,    MANAGED_BIT);
	SETUP_EVENT(UnitCmdDone,    MANAGED_BIT);
	SETUP_EVENT(UnitDamaged,    MANAGED_BIT);
	SETUP_EVENT(UnitExperience, MANAGED_BIT);

	SETUP_EVENT(UnitSeismicPing,  MANAGED_BIT);
	SETUP_EVENT(UnitEnteredRadar, MANAGED_BIT);
	SETUP_EVENT(UnitEnteredLos,   MANAGED_BIT);
	SETUP_EVENT(UnitLeftRadar,    MANAGED_BIT);
	SETUP_EVENT(UnitLeftLos,      MANAGED_BIT);

	SETUP_EVENT(UnitEnteredWater, MANAGED_BIT);
	SETUP_EVENT(UnitEnteredAir,   MANAGED_BIT);
	SETUP_EVENT(UnitLeftWater,    MANAGED_BIT);
	SETUP_EVENT(UnitLeftAir,      MANAGED_BIT);

	SETUP_EVENT(UnitLoaded,     MANAGED_BIT);
	SETUP_EVENT(UnitUnloaded,   MANAGED_BIT);
	SETUP_EVENT(UnitCloaked,    MANAGED_BIT);
	SETUP_EVENT(UnitDecloaked,  MANAGED_BIT);

	SETUP_EVENT(UnitUnitCollision,    MANAGED_BIT);
	SETUP_EVENT(UnitFeatureCollision, MANAGED_BIT);
	SETUP_EVENT(UnitMoved,            MANAGED_BIT);
	SETUP_EVENT(UnitMoveFailed,       MANAGED_BIT);

	SETUP_EVENT(FeatureCreated,   MANAGED_BIT);
	SETUP_EVENT(FeatureDestroyed, MANAGED_BIT);
	SETUP_EVENT(FeatureMoved,     MANAGED_BIT);

	SETUP_EVENT(ProjectileCreated,   MANAGED_BIT);
	SETUP_EVENT(ProjectileDestroyed, MANAGED_BIT);

	SETUP_EVENT(Explosion, MANAGED_BIT);

	SETUP_EVENT(StockpileChanged, MANAGED_BIT);

	// unsynced call-ins
	SETUP_EVENT(Save,           MANAGED_BIT | UNSYNCED_BIT);

	SETUP_EVENT(UnsyncedHeightMapUpdate, MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(Update,         MANAGED_BIT | UNSYNCED_BIT);

	SETUP_EVENT(KeyPress,       MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(KeyRelease,     MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(MouseMove,      MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(MousePress,     MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(MouseRelease,   MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(MouseWheel,     MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(JoystickEvent,  MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(IsAbove,        MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(GetTooltip,     MANAGED_BIT | UNSYNCED_BIT);

	SETUP_EVENT(DefaultCommand, MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(CommandNotify,  MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(AddConsoleLine, MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(LastMessagePosition, MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(GroupChanged,   MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(GameSetup,      MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(WorldTooltip,   MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(MapDrawCmd,     MANAGED_BIT | UNSYNCED_BIT);

	SETUP_EVENT(SunChanged,     MANAGED_BIT | UNSYNCED_BIT); //FIXME make synced? (whole dynSun code needs to then)

	SETUP_EVENT(ViewResize,     MANAGED_BIT | UNSYNCED_BIT);

	SETUP_EVENT(DrawGenesis,         MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(DrawWorld,           MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(DrawWorldPreUnit,    MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(DrawWorldShadow,     MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(DrawWorldReflection, MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(DrawWorldRefraction, MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(DrawScreenEffects,   MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(DrawScreen,          MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(DrawInMiniMap,       MANAGED_BIT | UNSYNCED_BIT);

	SETUP_EVENT(RenderUnitCreated,      MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(RenderUnitDestroyed,    MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(RenderUnitCloakChanged, MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(RenderUnitLOSChanged,   MANAGED_BIT | UNSYNCED_BIT);

	SETUP_EVENT(RenderFeatureCreated,   MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(RenderFeatureDestroyed, MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(RenderFeatureMoved,     MANAGED_BIT | UNSYNCED_BIT);

	SETUP_EVENT(RenderProjectileCreated,   MANAGED_BIT | UNSYNCED_BIT);
	SETUP_EVENT(RenderProjectileDestroyed, MANAGED_BIT | UNSYNCED_BIT);

	SETUP_EVENT(GameProgress,  MANAGED_BIT | UNSYNCED_BIT);

	// unmanaged call-ins
	SetupEvent("Shutdown", NULL, 0);
	SetupEvent("RecvLuaMsg", NULL, 0);

	SetupEvent("DrawUnit", NULL, UNSYNCED_BIT);
	SetupEvent("DrawFeature", NULL, UNSYNCED_BIT);
	SetupEvent("RecvSkirmishAIMessage", NULL, UNSYNCED_BIT);

	// LuaIntro
	SetupEvent("DrawLoadScreen",  NULL, UNSYNCED_BIT);
	SetupEvent("LoadProgress",  NULL, UNSYNCED_BIT);

	// LuaUI
	SetupEvent("ConfigureLayout", NULL, UNSYNCED_BIT | CONTROL_BIT);

	// LuaRules
	SetupEvent("CommandFallback",        NULL, CONTROL_BIT);
	SetupEvent("AllowCommand",           NULL, CONTROL_BIT);
	SetupEvent("AllowUnitCreation",      NULL, CONTROL_BIT);
	SetupEvent("AllowUnitTransfer",      NULL, CONTROL_BIT);
	SetupEvent("AllowUnitBuildStep",     NULL, CONTROL_BIT);
	SetupEvent("AllowFeatureCreation",   NULL, CONTROL_BIT);
	SetupEvent("AllowFeatureBuildStep",  NULL, CONTROL_BIT);
	SetupEvent("AllowResourceLevel",     NULL, CONTROL_BIT);
	SetupEvent("AllowResourceTransfer",  NULL, CONTROL_BIT);
	SetupEvent("AllowDirectUnitControl", NULL, CONTROL_BIT);
	SetupEvent("AllowStartPosition",     NULL, CONTROL_BIT);
	SetupEvent("TerraformComplete",      NULL, CONTROL_BIT);
	SetupEvent("MoveCtrlNotify",         NULL, CONTROL_BIT);
	SetupEvent("UnitPreDamaged",         NULL, CONTROL_BIT);
	SetupEvent("ShieldPreDamaged",       NULL, CONTROL_BIT);
	SetupEvent("AllowWeaponTargetCheck", NULL, CONTROL_BIT);
	SetupEvent("AllowWeaponTarget",      NULL, CONTROL_BIT);
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
	for (size_t i = 0; i < ecList.size(); i++) {
		if (ec != ecList[i]) {
			newList.push_back(ecList[i]);
		}
	}
	ecList = newList;
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

void CEventHandler::Load(IArchive* archive)
{
	ITERATE_EVENTCLIENTLIST(Load, archive);
}


#ifdef USE_GML
	#define GML_DRAW_CALLIN_SELECTOR() if(!globalConfig->enableDrawCallIns) return
#else
	#define GML_DRAW_CALLIN_SELECTOR()
#endif

#define GML_CALLIN_MUTEXES() \
	GML_THRMUTEX_LOCK(feat, GML_DRAW); \
	GML_THRMUTEX_LOCK(unit, GML_DRAW)/*; \
	GML_THRMUTEX_LOCK(proj, GML_DRAW)*/


#define EVENTHANDLER_CHECK(name, ...) \
	const int count = list ## name.size(); \
	if (count <= 0) \
		return __VA_ARGS__; \
	GML_CALLIN_MUTEXES()


void CEventHandler::Update()
{
	GML_DRAW_CALLIN_SELECTOR();

	EVENTHANDLER_CHECK(Update);

	ITERATE_EVENTCLIENTLIST(Update);
}



void CEventHandler::UpdateUnits() { eventBatchHandler->UpdateUnits(); }
void CEventHandler::UpdateDrawUnits() { eventBatchHandler->UpdateDrawUnits(); }
void CEventHandler::DeleteSyncedUnits() {
	eventBatchHandler->DeleteSyncedUnits();

	if (luaUI) luaUI->ExecuteUnitEventBatch();
}

void CEventHandler::UpdateFeatures() { eventBatchHandler->UpdateFeatures(); }
void CEventHandler::UpdateDrawFeatures() { eventBatchHandler->UpdateDrawFeatures(); }
void CEventHandler::DeleteSyncedFeatures() {
	eventBatchHandler->DeleteSyncedFeatures();

	if (luaUI) luaUI->ExecuteFeatEventBatch();
}

void CEventHandler::UpdateProjectiles() { eventBatchHandler->UpdateProjectiles(); }
void CEventHandler::UpdateDrawProjectiles() { eventBatchHandler->UpdateDrawProjectiles(); }

inline void ExecuteAllCallsFromSynced() {
#if (LUA_MT_OPT & LUA_MUTEX)
	bool exec;
	do { // these calls can be chained, need to purge them all
		exec = false;
		if (luaRules && luaRules->ExecuteCallsFromSynced())
			exec = true;
		if (luaGaia && luaGaia->ExecuteCallsFromSynced())
			exec = true;

		if (luaUI && luaUI->ExecuteCallsFromSynced())
			exec = true;
	} while (exec);
#endif
}

void CEventHandler::DeleteSyncedProjectiles() {
	ExecuteAllCallsFromSynced();
	eventBatchHandler->DeleteSyncedProjectiles();

	if (luaUI) luaUI->ExecuteProjEventBatch();
}

void CEventHandler::UpdateObjects() {
	eventBatchHandler->UpdateObjects();
}
void CEventHandler::DeleteSyncedObjects() {
	ExecuteAllCallsFromSynced();

	if (luaUI) luaUI->ExecuteObjEventBatch();
}



void CEventHandler::SunChanged(const float3& sunDir)
{
	EVENTHANDLER_CHECK(SunChanged);

	ITERATE_EVENTCLIENTLIST(SunChanged, sunDir);
}

void CEventHandler::ViewResize()
{
	EVENTHANDLER_CHECK(ViewResize);

	ITERATE_EVENTCLIENTLIST(ViewResize);
}


void CEventHandler::GameProgress(int gameFrame)
{
	ITERATE_EVENTCLIENTLIST(GameProgress, gameFrame);
}


#define DRAW_CALLIN(name)                         \
  void CEventHandler:: Draw ## name ()            \
  {                                               \
    GML_DRAW_CALLIN_SELECTOR();                   \
		                                          \
    EVENTHANDLER_CHECK(Draw ## name);             \
                                                  \
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


/******************************************************************************/
/******************************************************************************/

bool CEventHandler::CommandNotify(const Command& cmd)
{
	EVENTHANDLER_CHECK(CommandNotify, false);

	// reverse order, user has the override
	for (int i = (count - 1); i >= 0; i--) {
		CEventClient* ec = listCommandNotify[i];
		if (ec->CommandNotify(cmd)) {
			return true;
		}
	}
	return false;
}


bool CEventHandler::KeyPress(unsigned short key, bool isRepeat)
{
	EVENTHANDLER_CHECK(KeyPress, false);

	// reverse order, user has the override
	for (int i = (count - 1); i >= 0; i--) {
		CEventClient* ec = listKeyPress[i];
		if (ec->KeyPress(key, isRepeat)) {
			return true;
		}
	}
	return false;
}


bool CEventHandler::KeyRelease(unsigned short key)
{
	EVENTHANDLER_CHECK(KeyRelease, false);

	// reverse order, user has the override
	for (int i = (count - 1); i >= 0; i--) {
		CEventClient* ec = listKeyRelease[i];
		if (ec->KeyRelease(key)) {
			return true;
		}
	}
	return false;
}


bool CEventHandler::MousePress(int x, int y, int button)
{
	EVENTHANDLER_CHECK(MousePress, false);

	// reverse order, user has the override
	for (int i = (count - 1); i >= 0; i--) {
		CEventClient* ec = listMousePress[i];
		if (ec->MousePress(x, y, button)) {
			if (!mouseOwner)
				mouseOwner = ec;
			return true;
		}
	}
	return false;
}


// return a cmd index, or -1
int CEventHandler::MouseRelease(int x, int y, int button)
{
	if (mouseOwner == NULL) {
		return -1;
	}
	else
	{
		GML_CALLIN_MUTEXES();

		const int retval = mouseOwner->MouseRelease(x, y, button);
		mouseOwner = NULL;
		return retval;
	}
}


bool CEventHandler::MouseMove(int x, int y, int dx, int dy, int button)
{
	if (mouseOwner == NULL) {
		return false;
	}

	GML_CALLIN_MUTEXES();

	return mouseOwner->MouseMove(x, y, dx, dy, button);
}


bool CEventHandler::MouseWheel(bool up, float value)
{
	EVENTHANDLER_CHECK(MouseWheel, false);

	// reverse order, user has the override
	for (int i = (count - 1); i >= 0; i--) {
		CEventClient* ec = listMouseWheel[i];
		if (ec->MouseWheel(up, value)) {
			return true;
		}
	}
	return false;
}

bool CEventHandler::JoystickEvent(const std::string& event, int val1, int val2)
{
	EVENTHANDLER_CHECK(JoystickEvent, false);

	// reverse order, user has the override
	for (int i = (count - 1); i >= 0; i--) {
		CEventClient* ec = listMouseWheel[i];
		if (ec->JoystickEvent(event, val1, val2)) {
			return true;
		}
	}
	return false;
}

bool CEventHandler::IsAbove(int x, int y)
{
	EVENTHANDLER_CHECK(IsAbove, false);

	// reverse order, user has the override
	for (int i = (count - 1); i >= 0; i--) {
		CEventClient* ec = listIsAbove[i];
		if (ec->IsAbove(x, y)) {
			return true;
		}
	}
	return false;
}

string CEventHandler::GetTooltip(int x, int y)
{
	EVENTHANDLER_CHECK(GetTooltip, "");

	// reverse order, user has the override
	for (int i = (count - 1); i >= 0; i--) {
		CEventClient* ec = listGetTooltip[i];
		const string tt = ec->GetTooltip(x, y);
		if (!tt.empty()) {
			return tt;
		}
	}
	return "";
}


bool CEventHandler::AddConsoleLine(const std::string& msg, const std::string& section, int level)
{
	EVENTHANDLER_CHECK(AddConsoleLine, false);

	ITERATE_EVENTCLIENTLIST(AddConsoleLine, msg, section, level);

	return true;
}


void CEventHandler::LastMessagePosition(const float3& pos)
{
	EVENTHANDLER_CHECK(LastMessagePosition);

	ITERATE_EVENTCLIENTLIST(LastMessagePosition, pos);
}


bool CEventHandler::GroupChanged(int groupID)
{
	EVENTHANDLER_CHECK(GroupChanged, false);

	ITERATE_EVENTCLIENTLIST(GroupChanged, groupID);

	return false;
}



bool CEventHandler::GameSetup(const string& state, bool& ready,
                                  const map<int, string>& playerStates)
{
	EVENTHANDLER_CHECK(GameSetup, false);

	// reverse order, user has the override
	for (int i = (count - 1); i >= 0; i--) {
		CEventClient* ec = listGameSetup[i];
		if (ec->GameSetup(state, ready, playerStates)) {
			return true;
		}
	}
	return false;
}


string CEventHandler::WorldTooltip(const CUnit* unit,
                                   const CFeature* feature,
                                   const float3* groundPos)
{
	EVENTHANDLER_CHECK(WorldTooltip, "");

	// reverse order, user has the override
	for (int i = (count - 1); i >= 0; i--) {
		CEventClient* ec = listWorldTooltip[i];
		const string tt = ec->WorldTooltip(unit, feature, groundPos);
		if (!tt.empty()) {
			return tt;
		}
	}
	return "";
}


bool CEventHandler::MapDrawCmd(int playerID, int type,
                               const float3* pos0, const float3* pos1,
                                   const string* label)
{
	EVENTHANDLER_CHECK(MapDrawCmd, false);

	// reverse order, user has the override
	for (int i = (count - 1); i >= 0; i--) {
		CEventClient* ec = listMapDrawCmd[i];
		if (ec->MapDrawCmd(playerID, type, pos0, pos1, label)) {
			return true;
		}
	}
	return false;
}


/******************************************************************************/
/******************************************************************************/

