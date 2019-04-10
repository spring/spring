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

	for (const auto& element: eventMap) {
		const EventInfo& ei = element.second;

		if (!ei.HasPropBit(MANAGED_BIT))
			continue;

		if (!ec->WantsEvent(element.first))
			continue;

		InsertEvent(ec, element.first);
	}
}

void CEventHandler::RemoveClient(CEventClient* ec)
{
	if (mouseOwner == ec)
		mouseOwner = nullptr;

	ListRemove(handles, ec);

	for (const auto& element: eventMap) {
		const EventInfo& ei = element.second;

		if (!ei.HasPropBit(MANAGED_BIT))
			continue;

		RemoveEvent(ec, element.first);
	}
}


/******************************************************************************/
/******************************************************************************/

void CEventHandler::GetEventList(std::vector<std::string>& list) const
{
	list.clear();

	for (const auto& element: eventMap) {
		list.push_back(element.first);
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

		if (ec->GetOrder() < ecIt->GetOrder()) {
			ecList.insert(it, ec);
			return;
		}
		// should not happen
		if ((ec->GetOrder() == ecIt->GetOrder()) && (ec->GetName() < ecIt->GetName())) {
			ecList.insert(it, ec);
			return;
		}
	}

	ecList.push_back(ec);
}

void CEventHandler::ListRemove(EventClientList& ecList, CEventClient* ec)
{
	const auto ecIt = std::find(ecList.begin(), ecList.end(), ec);

	// erase does not accept end()
	if (ecIt == ecList.end())
		return;

	ecList.erase(ecIt);
}


/******************************************************************************/
/******************************************************************************/

template<typename T, typename F, typename... A> bool ControlIterateDefTrue(T& list, const F& func, A&&... args) {
	bool result = true;

	for (size_t i = 0; i < list.size(); ) {
		CEventClient* ec = list[i];

		result &= (ec->*func)(std::forward<A>(args)...);

		// the call-in may remove itself from the list
		i += (i < list.size() && ec == list[i]);
	}

	return result;
}

template<typename T, typename F, typename... A> bool ControlIterateDefFalse(T& list, const F& func, A&&... args) {
	bool result = false;

	for (size_t i = 0; i < list.size(); ) {
		CEventClient* ec = list[i];

		result |= (ec->*func)(std::forward<A>(args)...);

		// the call-in may remove itself from the list
		i += (i < list.size() && ec == list[i]);
	}

	return result;
}



bool CEventHandler::CommandFallback(const CUnit* unit, const Command& cmd)
{
	return ControlIterateDefTrue(listCommandFallback, &CEventClient::CommandFallback, unit, cmd);
}


bool CEventHandler::AllowCommand(const CUnit* unit, const Command& cmd, bool fromSynced)
{
	return ControlIterateDefTrue(listAllowCommand, &CEventClient::AllowCommand, unit, cmd, fromSynced);
}


bool CEventHandler::AllowUnitCreation(const UnitDef* unitDef, const CUnit* builder, const BuildInfo* buildInfo)
{
	return ControlIterateDefTrue(listAllowUnitCreation, &CEventClient::AllowUnitCreation, unitDef, builder, buildInfo);
}

bool CEventHandler::AllowUnitTransfer(const CUnit* unit, int newTeam, bool capture)
{
	return ControlIterateDefTrue(listAllowUnitTransfer, &CEventClient::AllowUnitTransfer, unit, newTeam, capture);
}

bool CEventHandler::AllowUnitBuildStep(const CUnit* builder, const CUnit* unit, float part)
{
	return ControlIterateDefTrue(listAllowUnitBuildStep, &CEventClient::AllowUnitBuildStep, builder, unit, part);
}

bool CEventHandler::AllowUnitTransport(const CUnit* transporter, const CUnit* transportee)
{
	return ControlIterateDefTrue(listAllowUnitTransport, &CEventClient::AllowUnitTransport, transporter, transportee);
}

bool CEventHandler::AllowUnitTransportLoad(const CUnit* transporter, const CUnit* transportee, const float3& loadPos, bool allowed)
{
	return ControlIterateDefTrue(listAllowUnitTransportLoad, &CEventClient::AllowUnitTransportLoad, transporter, transportee, loadPos, allowed);
}

bool CEventHandler::AllowUnitTransportUnload(const CUnit* transporter, const CUnit* transportee, const float3& unloadPos, bool allowed)
{
	return ControlIterateDefTrue(listAllowUnitTransportUnload, &CEventClient::AllowUnitTransportUnload, transporter, transportee, unloadPos, allowed);
}

bool CEventHandler::AllowUnitCloak(const CUnit* unit, const CUnit* enemy)
{
	return ControlIterateDefTrue(listAllowUnitCloak, &CEventClient::AllowUnitCloak, unit, enemy);
}

bool CEventHandler::AllowUnitDecloak(const CUnit* unit, const CSolidObject* object, const CWeapon* weapon)
{
	return ControlIterateDefTrue(listAllowUnitDecloak, &CEventClient::AllowUnitDecloak, unit, object, weapon);
}

bool CEventHandler::AllowUnitKamikaze(const CUnit* unit, const CUnit* target, bool allowed)
{
	return ControlIterateDefTrue(listAllowUnitKamikaze, &CEventClient::AllowUnitKamikaze, unit, target, allowed);
}


bool CEventHandler::AllowFeatureCreation(const FeatureDef* featureDef, int allyTeamID, const float3& pos)
{
	return ControlIterateDefTrue(listAllowFeatureCreation, &CEventClient::AllowFeatureCreation, featureDef, allyTeamID, pos);
}


bool CEventHandler::AllowFeatureBuildStep(const CUnit* builder, const CFeature* feature, float part)
{
	return ControlIterateDefTrue(listAllowFeatureBuildStep, &CEventClient::AllowFeatureBuildStep, builder, feature, part);
}


bool CEventHandler::AllowResourceLevel(int teamID, const std::string& type, float level)
{
	return ControlIterateDefTrue(listAllowResourceLevel, &CEventClient::AllowResourceLevel, teamID, type, level);
}


bool CEventHandler::AllowResourceTransfer(int oldTeam, int newTeam, const char* type, float amount)
{
	return ControlIterateDefTrue(listAllowResourceTransfer, &CEventClient::AllowResourceTransfer, oldTeam, newTeam, type, amount);
}


bool CEventHandler::AllowDirectUnitControl(int playerID, const CUnit* unit)
{
	return ControlIterateDefTrue(listAllowDirectUnitControl, &CEventClient::AllowDirectUnitControl, playerID, unit);
}


bool CEventHandler::AllowBuilderHoldFire(const CUnit* unit, int action)
{
	return ControlIterateDefTrue(listAllowBuilderHoldFire, &CEventClient::AllowBuilderHoldFire, unit, action);
}


bool CEventHandler::AllowStartPosition(int playerID, int teamID, unsigned char readyState, const float3& clampedPos, const float3& rawPickPos)
{
	return ControlIterateDefTrue(listAllowStartPosition, &CEventClient::AllowStartPosition, playerID, teamID, readyState, clampedPos, rawPickPos);
}



bool CEventHandler::TerraformComplete(const CUnit* unit, const CUnit* build)
{
	return ControlIterateDefFalse(listTerraformComplete, &CEventClient::TerraformComplete, unit, build);
}


bool CEventHandler::MoveCtrlNotify(const CUnit* unit, int data)
{
	return ControlIterateDefFalse(listMoveCtrlNotify, &CEventClient::MoveCtrlNotify, unit, data);
}


int CEventHandler::AllowWeaponTargetCheck(unsigned int attackerID, unsigned int attackerWeaponNum, unsigned int attackerWeaponDefID)
{
	int result = -1;

	for (size_t i = 0; i < listAllowWeaponTargetCheck.size(); ) {
		CEventClient* ec = listAllowWeaponTargetCheck[i];

		result = std::max(result, ec->AllowWeaponTargetCheck(attackerID, attackerWeaponNum, attackerWeaponDefID));

		// the call-in may remove itself from the list
		i += (i < listAllowWeaponTargetCheck.size() && ec == listAllowWeaponTargetCheck[i]);
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
	return ControlIterateDefTrue(listAllowWeaponTarget, &CEventClient::AllowWeaponTarget, attackerID, targetID, attackerWeaponNum, attackerWeaponDefID, targetPriority);
}

bool CEventHandler::AllowWeaponInterceptTarget(const CUnit* interceptorUnit, const CWeapon* interceptorWeapon, const CProjectile* interceptorTarget)
{
	return ControlIterateDefTrue(listAllowWeaponInterceptTarget, &CEventClient::AllowWeaponInterceptTarget, interceptorUnit, interceptorWeapon, interceptorTarget);
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
	return ControlIterateDefFalse(listUnitPreDamaged, &CEventClient::UnitPreDamaged, unit, attacker, damage, weaponDefID, projectileID, paralyzer, newDamage, impulseMult);
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
	return ControlIterateDefFalse(listFeaturePreDamaged, &CEventClient::FeaturePreDamaged, feature, attacker, damage, weaponDefID, projectileID, newDamage, impulseMult);
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
	return ControlIterateDefFalse(listShieldPreDamaged, &CEventClient::ShieldPreDamaged, projectile, shieldEmitter, shieldCarrier, bounceProjectile, beamEmitter, beamCarrier, startPos, hitPos);
}


bool CEventHandler::SyncedActionFallback(const std::string& line, int playerID)
{
	for (size_t i = 0; i < listSyncedActionFallback.size(); ) {
		CEventClient* ec = listSyncedActionFallback[i];

		if (ec->SyncedActionFallback(line, playerID))
			return true;

		// the call-in may remove itself from the list
		i += (i < listSyncedActionFallback.size() && ec == listSyncedActionFallback[i]);
	}

	return false;
}


/******************************************************************************/
/******************************************************************************/

template<typename T, typename F, typename... A> void IterateEventClientList(T& list, const F& func, A&&... args) {
	for (size_t i = 0; i < list.size(); ) {
		CEventClient* ec = list[i];

		(ec->*func)(std::forward<A>(args)...);

		// the call-in may remove itself from the list
		i += (i < list.size() && ec == list[i]);
	}
}

// not usable: "pasting "::" and "Save" does not give a valid preprocessing token"
// #define ITERATE_EVENTCLIENTLIST(func, ...) IterateEventClientList(list ## func, &CEventClient:: ## func, __VA_ARGS__)
#define ITERATE_EVENTCLIENTLIST_NA(func) IterateEventClientList(list ## func, &CEventClient::func)
#define ITERATE_EVENTCLIENTLIST(func, ...) IterateEventClientList(list ## func, &CEventClient::func, __VA_ARGS__)


void CEventHandler::Save(zipFile archive)
{
	ITERATE_EVENTCLIENTLIST(Save, archive);
}

void CEventHandler::Load(IArchive* archive)
{
	ITERATE_EVENTCLIENTLIST(Load, archive);
}


void CEventHandler::GamePreload()
{
	ITERATE_EVENTCLIENTLIST_NA(GamePreload);
}

void CEventHandler::GameStart()
{
	ITERATE_EVENTCLIENTLIST_NA(GameStart);
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

void CEventHandler::GameProgress(int gameFrame)
{
	ITERATE_EVENTCLIENTLIST(GameProgress, gameFrame);
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
	ITERATE_EVENTCLIENTLIST_NA(CollectGarbage);
}

void CEventHandler::DbgTimingInfo(DbgTimingInfoType type, const spring_time start, const spring_time end)
{
	ITERATE_EVENTCLIENTLIST(DbgTimingInfo, type, start, end);
}

void CEventHandler::Pong(uint8_t pingTag, const spring_time pktSendTime, const spring_time pktRecvTime)
{
	ITERATE_EVENTCLIENTLIST(Pong, pingTag, pktSendTime, pktRecvTime);
}


void CEventHandler::Update()
{
	ITERATE_EVENTCLIENTLIST_NA(Update);
}



void CEventHandler::SunChanged()
{
	ITERATE_EVENTCLIENTLIST_NA(SunChanged);
}

void CEventHandler::ViewResize()
{
	ITERATE_EVENTCLIENTLIST_NA(ViewResize);
}


#define DRAW_CALLIN(name)                                                      \
	void CEventHandler:: Draw ## name ()                                       \
	{                                                                          \
		if (listDraw ## name.empty())                                          \
			return;                                                            \
                                                                               \
		LuaOpenGL::EnableDraw ## name ();                                      \
		listDraw ## name [0]->Draw ## name ();                                 \
                                                                               \
		for (size_t i = 1; i < listDraw ## name.size(); ) {                    \
			LuaOpenGL::ResetDraw ## name ();                                   \
                                                                               \
			CEventClient* ec = listDraw ## name [i];                           \
			ec-> Draw ## name ();                                              \
                                                                               \
			i += (i < listDraw ## name.size() && ec == listDraw ## name [i]);  \
		}                                                                      \
                                                                               \
		LuaOpenGL::DisableDraw ## name ();                                     \
  }


DRAW_CALLIN(Genesis)
DRAW_CALLIN(Water)
DRAW_CALLIN(Sky)
DRAW_CALLIN(Sun)
DRAW_CALLIN(Grass)
DRAW_CALLIN(Trees)
DRAW_CALLIN(World)
DRAW_CALLIN(WorldPreUnit)
DRAW_CALLIN(WorldPreParticles)
DRAW_CALLIN(WorldShadow)
DRAW_CALLIN(WorldReflection)
DRAW_CALLIN(WorldRefraction)
DRAW_CALLIN(GroundPreForward)
DRAW_CALLIN(GroundPostForward)
DRAW_CALLIN(GroundPreDeferred)
DRAW_CALLIN(GroundPostDeferred)
DRAW_CALLIN(UnitsPostDeferred)
DRAW_CALLIN(FeaturesPostDeferred)
DRAW_CALLIN(ScreenEffects)
DRAW_CALLIN(ScreenPost)
DRAW_CALLIN(Screen)
DRAW_CALLIN(InMiniMap)
DRAW_CALLIN(InMiniMapBackground)


#define DRAW_ENTITY_CALLIN(name, args, args2)                                 \
	bool CEventHandler:: Draw ## name args                                    \
	{                                                                         \
		bool skipEngineDrawing = false;                                       \
                                                                              \
		for (size_t i = 0; i < listDraw ## name.size(); ) {                   \
			CEventClient* ec = listDraw ## name [i];                          \
			skipEngineDrawing |= ec-> Draw ## name args2 ;                    \
                                                                              \
			i += (i < listDraw ## name.size() && ec == listDraw ## name [i]); \
		}                                                                     \
                                                                              \
		return skipEngineDrawing;                                             \
  }


DRAW_ENTITY_CALLIN(Unit, (const CUnit* unit), (unit))
DRAW_ENTITY_CALLIN(Feature, (const CFeature* feature), (feature))
DRAW_ENTITY_CALLIN(Shield, (const CUnit* unit, const CWeapon* weapon), (unit, weapon))
DRAW_ENTITY_CALLIN(Projectile, (const CProjectile* projectile), (projectile))
DRAW_ENTITY_CALLIN(Material, (const LuaMaterial* material), (material))

/******************************************************************************/
/******************************************************************************/

template<typename T, typename F, typename... A> bool ControlReverseIterateDefTrue(T& list, const F& func, A&&... args) {
	for (size_t i = 0; i < list.size(); i++) {
		CEventClient* ec = list[list.size() - 1 - i];

		if ((ec->*func)(std::forward<A>(args)...))
			return true;
	}

	return false;
}

template<typename T, typename F, typename... A> std::string ControlReverseIterateDefString(T& list, const F& func, A&&... args) {
	for (size_t i = 0; i < list.size(); i++) {
		CEventClient* ec = list[list.size() - 1 - i];

		std::string str = std::move((ec->*func)(std::forward<A>(args)...));

		if (str.empty())
			continue;

		return (std::move(str));
	}

	return {};
}

bool CEventHandler::CommandNotify(const Command& cmd)
{
	return ControlReverseIterateDefTrue(listCommandNotify, &CEventClient::CommandNotify, cmd);
}


bool CEventHandler::KeyPress(int key, bool isRepeat)
{
	return ControlReverseIterateDefTrue(listKeyPress, &CEventClient::KeyPress, key, isRepeat);
}

bool CEventHandler::KeyRelease(int key)
{
	return ControlReverseIterateDefTrue(listKeyRelease, &CEventClient::KeyRelease, key);
}


bool CEventHandler::TextInput(const std::string& utf8)
{
	return ControlReverseIterateDefTrue(listTextInput, &CEventClient::TextInput, utf8);
}

bool CEventHandler::TextEditing(const std::string& utf8, unsigned int start, unsigned int length)
{
	return ControlReverseIterateDefTrue(listTextEditing, &CEventClient::TextEditing, utf8, start, length);
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
	return ControlReverseIterateDefTrue(listMouseWheel, &CEventClient::MouseWheel, up, value);
}


bool CEventHandler::IsAbove(int x, int y)
{
	return ControlReverseIterateDefTrue(listIsAbove, &CEventClient::IsAbove, x, y);
}


std::string CEventHandler::GetTooltip(int x, int y)
{
	return ControlReverseIterateDefString(listGetTooltip, &CEventClient::GetTooltip, x, y);
}

std::string CEventHandler::WorldTooltip(const CUnit* unit, const CFeature* feature, const float3* groundPos)
{
	return ControlReverseIterateDefString(listWorldTooltip, &CEventClient::WorldTooltip, unit, feature, groundPos);
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



bool CEventHandler::GameSetup(
	const std::string& state,
	bool& ready,
	const std::vector< std::pair<int, std::string> >& playerStates
) {
	return ControlReverseIterateDefTrue(listGameSetup, &CEventClient::GameSetup, state, ready, playerStates);
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


bool CEventHandler::MapDrawCmd(
	int playerID,
	int type,
	const float3* pos0,
	const float3* pos1,
	const std::string* label
) {
	return ControlReverseIterateDefTrue(listMapDrawCmd, &CEventClient::MapDrawCmd, playerID, type, pos0, pos1, label);
}


/******************************************************************************/
/******************************************************************************/

void CEventHandler::MetalMapChanged(const int x, const int z)
{
	ITERATE_EVENTCLIENTLIST(MetalMapChanged, x, z);
}
