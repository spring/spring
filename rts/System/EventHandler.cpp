/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/EventHandler.h"

#include "Lua/LuaCallInCheck.h"
#include "Lua/LuaOpenGL.h"  // FIXME -- should be moved

#include "System/Config/ConfigHandler.h"
#include "System/Platform/Threading.h"
#include "System/GlobalConfig.h"

CEventHandler eventHandler;


/******************************************************************************/
/******************************************************************************/

void CEventHandler::SetupEvent(const std::string& eName, EventClientList* list, int props)
{
	assert(std::find_if(eventMap.cbegin(), eventMap.cend(), [&](const EventPair& p) { return (p.first == eName); }) == eventMap.cend());
	eventMap.push_back({eName, EventInfo(eName, list, props)});
}

/******************************************************************************/
/******************************************************************************/

CEventHandler::CEventHandler()
{
	ResetState();
}

void CEventHandler::ResetState()
{
	mouseOwner = nullptr;

	eventMap.clear();
	eventMap.reserve(64);
	handles.clear();
	handles.reserve(16);

	SetupEvents();
}

void CEventHandler::SetupEvents()
{
	#define SETUP_EVENT(name, props) SetupEvent(#name, &list ## name, props);
	#define SETUP_UNMANAGED_EVENT(name, props) SetupEvent(#name, NULL, props);
		#include "Events.def"
	#undef SETUP_UNMANAGED_EVENT
	#undef SETUP_EVENT

	// sort by name
	std::stable_sort(eventMap.begin(), eventMap.end(), [](const EventPair& a, const EventPair& b) { return (a.first < b.first); });
}


/******************************************************************************/
/******************************************************************************/

void CEventHandler::AddClient(CEventClient* ec)
{
	ListInsert(handles, ec);

	for (auto it = eventMap.cbegin(); it != eventMap.cend(); ++it) {
		const EventInfo& ei = it->second;

		if (ei.HasPropBit(MANAGED_BIT)) {
			if (ec->WantsEvent(it->first)) {
				InsertEvent(ec, it->first);
			}
		}
	}
}

void CEventHandler::RemoveClient(CEventClient* ec)
{
	if (mouseOwner == ec)
		mouseOwner = nullptr;

	ListRemove(handles, ec);

	for (auto it = eventMap.cbegin(); it != eventMap.cend(); ++it) {
		const EventInfo& ei = it->second;
		if (ei.HasPropBit(MANAGED_BIT)) {
			RemoveEvent(ec, it->first);
		}
	}
}


/******************************************************************************/
/******************************************************************************/

void CEventHandler::GetEventList(std::vector<std::string>& list) const
{
	list.clear();

	for (auto it = eventMap.cbegin(); it != eventMap.cend(); ++it) {
		list.push_back(it->first);
	}
}


bool CEventHandler::IsKnown(const std::string& eName) const
{
	// std::binary_search does not return an iterator
	const auto comp = [](const EventPair& a, const EventPair& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(eventMap.begin(), eventMap.end(), EventPair{eName, {}}, comp);
	return (iter != eventMap.end() && iter->first == eName);
}


bool CEventHandler::IsManaged(const std::string& eName) const
{
	const auto comp = [](const EventPair& a, const EventPair& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(eventMap.begin(), eventMap.end(), EventPair{eName, {}}, comp);
	return (iter != eventMap.end() && iter->second.HasPropBit(MANAGED_BIT) && iter->first == eName);
}


bool CEventHandler::IsUnsynced(const std::string& eName) const
{
	const auto comp = [](const EventPair& a, const EventPair& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(eventMap.begin(), eventMap.end(), EventPair{eName, {}}, comp);
	return (iter != eventMap.end() && iter->second.HasPropBit(UNSYNCED_BIT) && iter->first == eName);
}


bool CEventHandler::IsController(const std::string& eName) const
{
	const auto comp = [](const EventPair& a, const EventPair& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(eventMap.begin(), eventMap.end(), EventPair{eName, {}}, comp);
	return (iter != eventMap.end() && iter->second.HasPropBit(CONTROL_BIT) && iter->first == eName);
}


/******************************************************************************/

bool CEventHandler::InsertEvent(CEventClient* ec, const std::string& ciName)
{
	const auto comp = [](const EventPair& a, const EventPair& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(eventMap.begin(), eventMap.end(), EventPair{ciName, {}}, comp);

	if ((iter == eventMap.end()) || (iter->second.GetList() == nullptr) || (iter->first != ciName))
		return false;

	if (ec->GetSynced() && iter->second.HasPropBit(UNSYNCED_BIT))
		return false;

	ListInsert(*iter->second.GetList(), ec);
	return true;
}


bool CEventHandler::RemoveEvent(CEventClient* ec, const std::string& ciName)
{
	const auto comp = [](const EventPair& a, const EventPair& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(eventMap.begin(), eventMap.end(), EventPair{ciName, {}}, comp);

	if ((iter == eventMap.end()) || (iter->second.GetList() == nullptr) || (iter->first != ciName))
		return false;

	ListRemove(*(iter->second.GetList()), ec);
	return true;
}


/******************************************************************************/

void CEventHandler::ListInsert(EventClientList& ecList, CEventClient* ec)
{
	for (auto it = ecList.begin(); it != ecList.end(); ++it) {
		const CEventClient* ecIt = *it;

		if (ec == ecIt)
			return; // already in the list

		if ((ec->GetOrder()  <  ecIt->GetOrder()) ||
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

#define CONTROL_ITERATE_DEF_TRUE(name, ...)                        \
	bool result = true;                                            \
                                                                   \
	for (size_t i = 0; i < list##name.size(); ) {                  \
		CEventClient* ec = list##name[i];                          \
		result &= ec->name(__VA_ARGS__);                           \
                                                                   \
		if (i < list##name.size() && ec == list##name[i])          \
			++i; /* the call-in may remove itself from the list */ \
	}                                                              \
                                                                   \
	return result;

#define CONTROL_ITERATE_DEF_FALSE(name, ...)                       \
	bool result = false;                                           \
                                                                   \
	for (size_t i = 0; i < list##name.size(); ) {                  \
		CEventClient* ec = list##name[i];                          \
		result |= ec->name(__VA_ARGS__);                           \
                                                                   \
		if (i < list##name.size() && ec == list##name[i])          \
			++i; /* the call-in may remove itself from the list */ \
	}                                                              \
                                                                   \
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


bool CEventHandler::AllowResourceLevel(int teamID, const std::string& type, float level)
{
	CONTROL_ITERATE_DEF_TRUE(AllowResourceLevel, teamID, type, level)
}


bool CEventHandler::AllowResourceTransfer(int oldTeam, int newTeam, const std::string& type, float amount)
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


bool CEventHandler::AllowStartPosition(int playerID, int teamID, unsigned char readyState, const float3& clampedPos, const float3& rawPickPos)
{
	CONTROL_ITERATE_DEF_TRUE(AllowStartPosition, playerID, teamID, readyState, clampedPos, rawPickPos)
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

	for (size_t i = 0; i < listAllowWeaponTargetCheck.size(); ) {
		CEventClient* ec = listAllowWeaponTargetCheck[i];

		result = std::max(result, ec->AllowWeaponTargetCheck(attackerID, attackerWeaponNum, attackerWeaponDefID));

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


bool CEventHandler::ShieldPreDamaged(
	const CProjectile* projectile,
	const CWeapon* shieldEmitter,
	const CUnit* shieldCarrier,
	bool bounceProjectile,
	const CWeapon* beamEmitter,
	const CUnit* beamCarrier,
	const float3& startPos,
	const float3& hitPos
) {
	CONTROL_ITERATE_DEF_FALSE(ShieldPreDamaged, projectile, shieldEmitter, shieldCarrier, bounceProjectile, beamEmitter, beamCarrier, startPos, hitPos)
}


bool CEventHandler::SyncedActionFallback(const std::string& line, int playerID)
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

#define ITERATE_EVENTCLIENTLIST(name, ...)                         \
	for (size_t i = 0; i < list##name.size(); ) {                  \
		CEventClient* ec = list##name[i];                          \
		ec->name(__VA_ARGS__);                                     \
                                                                   \
		if (i < list##name.size() && ec == list##name[i])          \
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



void CEventHandler::SunChanged()
{
	ITERATE_EVENTCLIENTLIST(SunChanged);
}

void CEventHandler::ViewResize()
{
	ITERATE_EVENTCLIENTLIST(ViewResize);
}


void CEventHandler::GameProgress(int gameFrame)
{
	ITERATE_EVENTCLIENTLIST(GameProgress, gameFrame);
}


#define DRAW_CALLIN(name)                                                   \
	void CEventHandler:: Draw ## name ()                                    \
	{                                                                       \
		if (listDraw ## name.empty())                                       \
			return;                                                         \
                                                                            \
		LuaOpenGL::EnableDraw ## name ();                                   \
		listDraw ## name [0]->Draw ## name ();                              \
                                                                            \
		for (size_t i = 1; i < listDraw ## name.size(); ) {                 \
			LuaOpenGL::ResetDraw ## name ();                                \
			CEventClient* ec = listDraw ## name [i];                        \
			ec-> Draw ## name ();                                           \
                                                                            \
			if (i < listDraw ## name.size() && ec == listDraw ## name [i])  \
				++i;                                                        \
		}                                                                   \
                                                                            \
		LuaOpenGL::DisableDraw ## name ();                                  \
  }

DRAW_CALLIN(Genesis)
DRAW_CALLIN(World)
DRAW_CALLIN(WorldPreUnit)
DRAW_CALLIN(WorldPreParticles)
DRAW_CALLIN(WorldShadow)
DRAW_CALLIN(WorldReflection)
DRAW_CALLIN(WorldRefraction)
DRAW_CALLIN(GroundPreForward)
DRAW_CALLIN(GroundPreDeferred)
DRAW_CALLIN(GroundPostDeferred)
DRAW_CALLIN(UnitsPostDeferred)
DRAW_CALLIN(FeaturesPostDeferred)
DRAW_CALLIN(ScreenEffects)
DRAW_CALLIN(ScreenPost)
DRAW_CALLIN(Screen)
DRAW_CALLIN(InMiniMap)
DRAW_CALLIN(InMiniMapBackground)


#define DRAW_ENTITY_CALLIN(name, args, args2)                               \
	bool CEventHandler:: Draw ## name args                                  \
	{                                                                       \
		bool skipEngineDrawing = false;                                     \
                                                                            \
		for (size_t i = 0; i < listDraw ## name.size(); ) {                 \
			CEventClient* ec = listDraw ## name [i];                        \
			skipEngineDrawing |= ec-> Draw ## name args2 ;                  \
                                                                            \
			if (i < listDraw ## name.size() && ec == listDraw ## name [i])  \
				++i;                                                        \
		}                                                                   \
                                                                            \
		return skipEngineDrawing;                                           \
  }

DRAW_ENTITY_CALLIN(Unit, (const CUnit* unit), (unit))
DRAW_ENTITY_CALLIN(Feature, (const CFeature* feature), (feature))
DRAW_ENTITY_CALLIN(Shield, (const CUnit* unit, const CWeapon* weapon), (unit, weapon))
DRAW_ENTITY_CALLIN(Projectile, (const CProjectile* projectile), (projectile))

/******************************************************************************/
/******************************************************************************/

#define CONTROL_REVERSE_ITERATE_DEF_TRUE(name, ...)                \
	for (size_t i = 0; i < list##name.size(); ++i) {               \
		CEventClient* ec = list##name[list##name.size() - 1 - i];  \
                                                                   \
		if (ec->name(__VA_ARGS__))                                 \
			return true;                                           \
	}

#define CONTROL_REVERSE_ITERATE_STRING(name, ...)                  \
	for (size_t i = 0; i < list##name.size(); ++i) {               \
		CEventClient* ec = list##name[list##name.size() - 1 - i];  \
		const std::string& str = ec->name(__VA_ARGS__);            \
                                                                   \
		if (!str.empty())                                          \
			return str;                                            \
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
	for (size_t i = 0; i < listMousePress.size(); i++) {
		CEventClient* ec = listMousePress[listMousePress.size() - 1 - i];

		if (ec->MousePress(x, y, button)) {
			if (mouseOwner == nullptr)
				mouseOwner = ec;

			return true;
		}
	}

	return false;
}


void CEventHandler::MouseRelease(int x, int y, int button)
{
	if (mouseOwner == nullptr)
		return;

	mouseOwner->MouseRelease(x, y, button);
	mouseOwner = nullptr;
}


bool CEventHandler::MouseMove(int x, int y, int dx, int dy, int button)
{
	if (mouseOwner == nullptr)
		return false;

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

std::string CEventHandler::GetTooltip(int x, int y)
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



bool CEventHandler::GameSetup(const std::string& state, bool& ready,
                                  const std::vector< std::pair<int, std::string> >& playerStates)
{
	CONTROL_REVERSE_ITERATE_DEF_TRUE(GameSetup, state, ready, playerStates)
	return false;
}

void CEventHandler::DownloadQueued(int ID, const std::string& archiveName, const std::string& archiveType)
{
	ITERATE_EVENTCLIENTLIST(DownloadQueued, ID, archiveName, archiveType);
}

void CEventHandler::DownloadStarted(int ID)
{
	ITERATE_EVENTCLIENTLIST(DownloadStarted, ID);
}

void CEventHandler::DownloadFinished(int ID)
{
	ITERATE_EVENTCLIENTLIST(DownloadFinished, ID);
}

void CEventHandler::DownloadFailed(int ID, int errorID)
{
	ITERATE_EVENTCLIENTLIST(DownloadFailed, ID, errorID);
}

void CEventHandler::DownloadProgress(int ID, long downloaded, long total)
{
	ITERATE_EVENTCLIENTLIST(DownloadProgress, ID, downloaded, total);
}

std::string CEventHandler::WorldTooltip(const CUnit* unit,
                                   const CFeature* feature,
                                   const float3* groundPos)
{
	CONTROL_REVERSE_ITERATE_STRING(WorldTooltip, unit, feature, groundPos)
	return "";
}


bool CEventHandler::MapDrawCmd(int playerID, int type,
                               const float3* pos0, const float3* pos1,
                               const std::string* label)
{
	CONTROL_REVERSE_ITERATE_DEF_TRUE(MapDrawCmd, playerID, type, pos0, pos1, label)
	return false;
}


/******************************************************************************/
/******************************************************************************/

void CEventHandler::MetalMapChanged(const int x, const int z)
{
	ITERATE_EVENTCLIENTLIST(MetalMapChanged, x, z);
}

