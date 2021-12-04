/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <string>
#include <vector>

#include "System/EventClient.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/Projectile.h"

class CWeapon;
struct Command;
struct BuildInfo;
class LuaMaterial;

class CEventHandler
{
	public:
		CEventHandler();

		void ResetState();
		void SetupEvents();

		void AddClient(CEventClient* ec);
		void RemoveClient(CEventClient* ec);
		bool HasClient(CEventClient* ec) const {
			return (std::find(handles.begin(), handles.end(), ec) != handles.end());
		}

		bool InsertEvent(CEventClient* ec, const std::string& ciName);
		bool RemoveEvent(CEventClient* ec, const std::string& ciName);

		void GetEventList(std::vector<std::string>& list) const;

		bool IsKnown(const std::string& ciName) const;
		bool IsManaged(const std::string& ciName) const;
		bool IsUnsynced(const std::string& ciName) const;
		bool IsController(const std::string& ciName) const;


	public:
		/**
		 * @name Synced_events
		 * @{
		 */
		void Load(IArchive* archive);

		void GamePreload();
		void GameStart();
		void GameOver(const std::vector<unsigned char>& winningAllyTeams);
		void GamePaused(int playerID, bool paused);
		void GameFrame(int gameFrame);
		void GameID(const unsigned char* gameID, unsigned int numBytes);

		void TeamDied(int teamID);
		void TeamChanged(int teamID);
		void PlayerChanged(int playerID);
		void PlayerAdded(int playerID);
		void PlayerRemoved(int playerID, int reason);

		void UnitCreated(const CUnit* unit, const CUnit* builder);
		void UnitFinished(const CUnit* unit);
		void UnitReverseBuilt(const CUnit* unit);
		void UnitFromFactory(const CUnit* unit, const CUnit* factory, bool userOrders);
		void UnitDestroyed(const CUnit* unit, const CUnit* attacker);
		void UnitTaken(const CUnit* unit, int oldTeam, int newTeam);
		void UnitGiven(const CUnit* unit, int oldTeam, int newTeam);


		//FIXME no events
		void RenderUnitCreated(const CUnit* unit, int cloaked);
		void RenderUnitDestroyed(const CUnit* unit);
		void RenderFeatureCreated(const CFeature* feature);
		void RenderFeatureDestroyed(const CFeature* feature);
		void RenderProjectileCreated(const CProjectile* proj);
		void RenderProjectileDestroyed(const CProjectile* proj);

		void UnitIdle(const CUnit* unit);
		void UnitCommand(const CUnit* unit, const Command& command, int playerNum, bool fromSynced, bool fromLua);
		void UnitCmdDone(const CUnit* unit, const Command& command                                              );
		void UnitDamaged(
			const CUnit* unit,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID,
			bool paralyzer);
		void UnitStunned(const CUnit* unit, bool stunned);
		void UnitExperience(const CUnit* unit, float oldExperience);
		void UnitHarvestStorageFull(const CUnit* unit);

		void UnitSeismicPing(const CUnit* unit, int allyTeam,
		                     const float3& pos, float strength);
		void UnitEnteredRadar(const CUnit* unit, int allyTeam);
		void UnitEnteredLos(const CUnit* unit, int allyTeam);
		void UnitLeftRadar(const CUnit* unit, int allyTeam);
		void UnitLeftLos(const CUnit* unit, int allyTeam);

		void UnitEnteredWater(const CUnit* unit);
		void UnitEnteredAir(const CUnit* unit);
		void UnitLeftWater(const CUnit* unit);
		void UnitLeftAir(const CUnit* unit);

		void UnitLoaded(const CUnit* unit, const CUnit* transport);
		void UnitUnloaded(const CUnit* unit, const CUnit* transport);

		void UnitCloaked(const CUnit* unit);
		void UnitDecloaked(const CUnit* unit);

		bool UnitUnitCollision(const CUnit* collider, const CUnit* collidee);
		bool UnitFeatureCollision(const CUnit* collider, const CFeature* collidee);
		void UnitMoved(const CUnit* unit);
		void UnitMoveFailed(const CUnit* unit);

		void FeatureCreated(const CFeature* feature);
		void FeatureDestroyed(const CFeature* feature);
		void FeatureDamaged(
			const CFeature* feature,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID);
		void FeatureMoved(const CFeature* feature, const float3& oldpos);

		void ProjectileCreated(const CProjectile* proj, int allyTeam);
		void ProjectileDestroyed(const CProjectile* proj, int allyTeam);

		bool Explosion(int weaponDefID, int projectileID, const float3& pos, const CUnit* owner);

		void StockpileChanged(const CUnit* unit,
		                      const CWeapon* weapon, int oldCount);

		bool CommandFallback(const CUnit* unit, const Command& cmd);
		bool AllowCommand(const CUnit* unit, const Command& cmd, int playerNum, bool fromSynced, bool fromLua);

		bool AllowUnitCreation(const UnitDef* unitDef, const CUnit* builder, const BuildInfo* buildInfo);
		bool AllowUnitTransfer(const CUnit* unit, int newTeam, bool capture);
		bool AllowUnitBuildStep(const CUnit* builder, const CUnit* unit, float part);
		bool AllowUnitTransport(const CUnit* transporter, const CUnit* transportee);
		bool AllowUnitTransportLoad(const CUnit* transporter, const CUnit* transportee, const float3& loadPos, bool allowed);
		bool AllowUnitTransportUnload(const CUnit* transporter, const CUnit* transportee, const float3& unloadPos, bool allowed);
		bool AllowUnitCloak(const CUnit* unit, const CUnit* enemy);
		bool AllowUnitDecloak(const CUnit* unit, const CSolidObject* object, const CWeapon* weapon);
		bool AllowUnitKamikaze(const CUnit* unit, const CUnit* target, bool allowed);
		bool AllowFeatureCreation(const FeatureDef* featureDef, int allyTeamID, const float3& pos);
		bool AllowFeatureBuildStep(const CUnit* builder, const CFeature* feature, float part);
		bool AllowResourceLevel(int teamID, const string& type, float level);
		bool AllowResourceTransfer(int oldTeam, int newTeam, const char* type, float amount);
		bool AllowDirectUnitControl(int playerID, const CUnit* unit);
		bool AllowBuilderHoldFire(const CUnit* unit, int action);
		bool AllowStartPosition(int playerID, int teamID, unsigned char readyState, const float3& clampedPos, const float3& rawPickPos);

		bool TerraformComplete(const CUnit* unit, const CUnit* build);
		bool MoveCtrlNotify(const CUnit* unit, int data);

		int AllowWeaponTargetCheck(unsigned int attackerID, unsigned int attackerWeaponNum, unsigned int attackerWeaponDefID);
		bool AllowWeaponTarget(
			unsigned int attackerID,
			unsigned int targetID,
			unsigned int attackerWeaponNum,
			unsigned int attackerWeaponDefID,
			float* targetPriority
		);
		bool AllowWeaponInterceptTarget(const CUnit* interceptorUnit, const CWeapon* interceptorWeapon, const CProjectile* interceptorTarget);

		bool UnitPreDamaged(
			const CUnit* unit,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID,
			bool paralyzer,
			float* newDamage,
			float* impulseMult
		);

		bool FeaturePreDamaged(
			const CFeature* feature,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID,
			float* newDamage,
			float* impulseMult
		);

		bool ShieldPreDamaged(
			const CProjectile* projectile,
			const CWeapon* shieldEmitter,
			const CUnit* shieldCarrier,
			bool bounceProjectile,
			const CWeapon* beamEmitter,
			const CUnit* beamCarrier,
			const float3& startPos,
			const float3& hitPos
		);

		bool SyncedActionFallback(const string& line, int playerID);
		/// @}

	public:
		/**
		 * @name Unsynced_events
		 * @{
		 */
		void Save(zipFile archive);

		void UnsyncedHeightMapUpdate(const SRectangle& rect);
		void Update();

		bool KeyPress(int key, bool isRepeat);
		bool KeyRelease(int key);
		bool TextInput(const std::string& utf8);
		bool TextEditing(const std::string& utf8, unsigned int start, unsigned int length);
		bool MouseMove(int x, int y, int dx, int dy, int button);
		bool MousePress(int x, int y, int button);
		void MouseRelease(int x, int y, int button);
		bool MouseWheel(bool up, float value);
		bool IsAbove(int x, int y);

		std::string GetTooltip(int x, int y);

		void DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd);

		bool CommandNotify(const Command& cmd);

		bool AddConsoleLine(const std::string& msg, const std::string& section, int level);

		void LastMessagePosition(const float3& pos);

		bool GroupChanged(int groupID);

		bool GameSetup(const std::string& state, bool& ready,
		               const std::vector< std::pair<int, std::string> >& playerStates);
		void DownloadQueued(int ID, const string& archiveName, const string& archiveType);
		void DownloadStarted(int ID);
		void DownloadFinished(int ID);
		void DownloadFailed(int ID, int errorID);
		void DownloadProgress(int ID, long downloaded, long total);

		std::string WorldTooltip(const CUnit* unit,
		                         const CFeature* feature,
		                         const float3* groundPos);

		bool MapDrawCmd(int playerID, int type,
		                const float3* pos0,
		                const float3* pos1,
		                const std::string* label);

		void SunChanged();

		void ViewResize();

		// see the DRAW_CALLIN macro for implementations
		void DrawGenesis();
		void DrawWater();
		void DrawSky();
		void DrawSun();
		void DrawGrass();
		void DrawTrees();
		void DrawWorld();
		void DrawWorldPreUnit();
		void DrawWorldPreParticles();
		void DrawWorldShadow();
		void DrawWorldReflection();
		void DrawWorldRefraction();
		void DrawGroundPreForward();
		void DrawGroundPostForward();
		void DrawGroundPreDeferred();
		void DrawGroundPostDeferred();
		void DrawUnitsPostDeferred();
		void DrawFeaturesPostDeferred();
		void DrawScreenPost();
		void DrawScreenEffects();
		void DrawScreen();
		void DrawInMiniMap();
		void DrawInMiniMapBackground();

		bool DrawUnit(const CUnit* unit);
		bool DrawFeature(const CFeature* feature);
		bool DrawShield(const CUnit* unit, const CWeapon* weapon);
		bool DrawProjectile(const CProjectile* projectile);
		bool DrawMaterial(const LuaMaterial* material);

		/// @brief this UNSYNCED event is generated every GameServer::gameProgressFrameInterval
		/// it skips network queuing and caching and can be used to calculate the current catchup
		/// percentage when reconnecting to a running game
		void GameProgress(int gameFrame);

		void CollectGarbage(bool forced);
		void DbgTimingInfo(DbgTimingInfoType type, const spring_time start, const spring_time end);
		void Pong(uint8_t pingTag, const spring_time pktSendTime, const spring_time pktRecvTime);
		void MetalMapChanged(const int x, const int z);
		/// @}

	private:
		typedef std::vector<CEventClient*> EventClientList;

		enum EventPropertyBits {
			MANAGED_BIT  = (1 << 0), // managed by eventHandler
			UNSYNCED_BIT = (1 << 1), // delivers unsynced information
			CONTROL_BIT  = (1 << 2)  // controls synced information
		};

		class EventInfo {
			public:
				EventInfo() : list(NULL), propBits(0) {}
				EventInfo(const std::string& _name, EventClientList* _list, int _bits)
				: name(_name), list(_list), propBits(_bits) {}
				~EventInfo() {}

				inline const std::string& GetName() const { return name; }
				inline EventClientList* GetList() const { return list; }
				inline int GetPropBits() const { return propBits; }
				inline bool HasPropBit(int bit) const { return propBits & bit; }

			private:
				std::string name;
				EventClientList* list;
				int propBits;
		};

		typedef std::pair<std::string, EventInfo> EventPair;
		typedef std::vector<EventPair> EventMap;

	private:
		void SetupEvent(const std::string& ciName,
		                EventClientList* list, int props);
		void ListInsert(EventClientList& ciList, CEventClient* ec);
		void ListRemove(EventClientList& ciList, CEventClient* ec);

	private:
		CEventClient* mouseOwner;

	private:
		EventMap eventMap;

		EventClientList handles;

	#define SETUP_EVENT(name, props) EventClientList list ## name;
	#define SETUP_UNMANAGED_EVENT(name, props)
		#include "Events.def"
	#undef SETUP_EVENT
	#undef SETUP_UNMANAGED_EVENT
};


extern CEventHandler eventHandler;


/******************************************************************************/
/******************************************************************************/
//
// Inlined call-in loops
//

#define ITERATE_EVENTCLIENTLIST(name, ...)                         \
	for (size_t i = 0; i < list##name.size(); ) {                  \
		CEventClient* ec = list##name[i];                          \
		ec->name(__VA_ARGS__);                                     \
                                                                   \
		/* the call-in may remove itself from the list */          \
		i += (i < list##name.size() && ec == list##name[i]);       \
	}

#define ITERATE_ALLYTEAM_EVENTCLIENTLIST(name, allyTeam, ...)      \
	for (size_t i = 0; i < list##name.size(); ) {                  \
		CEventClient* ec = list##name[i];                          \
                                                                   \
		if (ec->CanReadAllyTeam(allyTeam))                         \
			ec->name(__VA_ARGS__);                                 \
                                                                   \
		/* the call-in may remove itself from the list */          \
		i += (i < list##name.size() && ec == list##name[i]);       \
	}

#define ITERATE_UNIT_ALLYTEAM_EVENTCLIENTLIST(name, unit, ...)     \
	const auto unitAllyTeam = unit->allyteam;                      \
	for (size_t i = 0; i < list##name.size(); ) {                  \
		CEventClient* ec = list##name[i];                          \
                                                                   \
		if (ec->CanReadAllyTeam(unitAllyTeam))                     \
			ec->name(unit, __VA_ARGS__);                           \
                                                                   \
		/* the call-in may remove itself from the list */          \
		i += (i < list##name.size() && ec == list##name[i]);       \
	}

inline void CEventHandler::UnitCreated(const CUnit* unit, const CUnit* builder)
{
	ITERATE_UNIT_ALLYTEAM_EVENTCLIENTLIST(UnitCreated, unit, builder)
}


inline void CEventHandler::UnitDestroyed(const CUnit* unit, const CUnit* attacker)
{
	ITERATE_UNIT_ALLYTEAM_EVENTCLIENTLIST(UnitDestroyed, unit, attacker)
}

#define UNIT_CALLIN_NO_PARAM(name)                                 \
	inline void CEventHandler:: name (const CUnit* unit)           \
	{                                                              \
		const auto unitAllyTeam = unit->allyteam;                  \
		for (size_t i = 0; i < list##name.size(); ) {              \
			CEventClient* ec = list##name[i];                      \
                                                                   \
			if (ec->CanReadAllyTeam(unitAllyTeam))                 \
				ec->name(unit);                                    \
                                                                   \
			i += (i < list##name.size() && ec == list##name[i]);   \
		}                                                          \
	}

UNIT_CALLIN_NO_PARAM(UnitReverseBuilt);
UNIT_CALLIN_NO_PARAM(UnitFinished)
UNIT_CALLIN_NO_PARAM(UnitIdle)
UNIT_CALLIN_NO_PARAM(UnitMoveFailed)
UNIT_CALLIN_NO_PARAM(UnitEnteredWater)
UNIT_CALLIN_NO_PARAM(UnitEnteredAir)
UNIT_CALLIN_NO_PARAM(UnitLeftWater)
UNIT_CALLIN_NO_PARAM(UnitLeftAir)
UNIT_CALLIN_NO_PARAM(UnitMoved)

#define UNIT_CALLIN_INT_PARAMS(name)                                              \
	inline void CEventHandler:: Unit ## name (const CUnit* unit, int p1, int p2)  \
	{                                                                             \
		ITERATE_UNIT_ALLYTEAM_EVENTCLIENTLIST(Unit ## name, unit, p1, p2)         \
	}

UNIT_CALLIN_INT_PARAMS(Taken)
UNIT_CALLIN_INT_PARAMS(Given)


#define UNIT_CALLIN_LOS_PARAM(name)                                        \
	inline void CEventHandler:: Unit ## name (const CUnit* unit, int at)   \
	{                                                                      \
		ITERATE_ALLYTEAM_EVENTCLIENTLIST(Unit ## name, at, unit, at)       \
	}

UNIT_CALLIN_LOS_PARAM(EnteredRadar)
UNIT_CALLIN_LOS_PARAM(EnteredLos)
UNIT_CALLIN_LOS_PARAM(LeftRadar)
UNIT_CALLIN_LOS_PARAM(LeftLos)



inline void CEventHandler::UnitFromFactory(const CUnit* unit,
                                               const CUnit* factory,
                                               bool userOrders)
{
	ITERATE_UNIT_ALLYTEAM_EVENTCLIENTLIST(UnitFromFactory, unit, factory, userOrders)
}


UNIT_CALLIN_NO_PARAM(UnitCloaked)
UNIT_CALLIN_NO_PARAM(UnitDecloaked)


inline bool CEventHandler::UnitUnitCollision(const CUnit* collider, const CUnit* collidee)
{
	auto& clients = listUnitUnitCollision;

	for (size_t i = 0; i < clients.size(); ) {
		CEventClient* ec = clients[i];

		// discard return-value from clients lacking full-read access
		// (redundant for synced gadgets; watchWeaponDefs is checked)
		// NOTE: the call-in may remove itself from the client list
		if (!ec->UnitUnitCollision(collider, collidee) || !ec->GetFullRead()) {
			i += (i < clients.size() && ec == clients[i]);
			continue;
		}

		return true;
	}

	return false;
}

inline bool CEventHandler::UnitFeatureCollision(const CUnit* collider, const CFeature* collidee)
{
	auto& clients = listUnitFeatureCollision;

	for (size_t i = 0; i < clients.size(); ) {
		CEventClient* ec = clients[i];

		// discard return-value from clients lacking full-read access
		// (redundant for synced gadgets; watch{U,F}*Defs is checked)
		// NOTE: the call-in may remove itself from the client list
		if (!ec->UnitFeatureCollision(collider, collidee) || !ec->GetFullRead()) {
			i += (i < clients.size() && ec == clients[i]);
			continue;
		}

		return true;
	}

	return false;
}



inline void CEventHandler::UnitCommand(const CUnit* unit, const Command& command, int playerNum, bool fromSynced, bool fromLua)
{
	ITERATE_UNIT_ALLYTEAM_EVENTCLIENTLIST(UnitCommand, unit, command, playerNum, fromSynced, fromLua)
}

inline void CEventHandler::UnitCmdDone(const CUnit* unit, const Command& command)
{
	ITERATE_UNIT_ALLYTEAM_EVENTCLIENTLIST(UnitCmdDone, unit, command)
}


inline void CEventHandler::UnitDamaged(
	const CUnit* unit,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID,
	bool paralyzer)
{
	ITERATE_UNIT_ALLYTEAM_EVENTCLIENTLIST(UnitDamaged, unit, attacker, damage, weaponDefID, projectileID, paralyzer)
}

inline void CEventHandler::UnitStunned(
	const CUnit* unit,
	bool stunned)
{
	ITERATE_UNIT_ALLYTEAM_EVENTCLIENTLIST(UnitStunned, unit, stunned)
}


inline void CEventHandler::UnitExperience(const CUnit* unit,
                                              float oldExperience)
{
	ITERATE_UNIT_ALLYTEAM_EVENTCLIENTLIST(UnitExperience, unit, oldExperience)
}


inline void  CEventHandler::UnitSeismicPing(const CUnit* unit,
                                                int allyTeam,
                                                const float3& pos,
                                                float strength)
{
	ITERATE_UNIT_ALLYTEAM_EVENTCLIENTLIST(UnitSeismicPing, unit, allyTeam, pos, strength)
}


inline void CEventHandler::UnitLoaded(const CUnit* unit,
                                          const CUnit* transport)
{
	const size_t count = listUnitLoaded.size();

	for (size_t i = 0; i < count; i++) {
		CEventClient* ec = listUnitLoaded[i];
		const int ecAllyTeam = ec->GetReadAllyTeam();
		if (ec->GetFullRead() ||
		    (ecAllyTeam == unit->allyteam) ||
		    (ecAllyTeam == transport->allyteam)) {
			ec->UnitLoaded(unit, transport);
		}
	}
}


inline void CEventHandler::UnitUnloaded(const CUnit* unit,
                                            const CUnit* transport)
{
	const size_t count = listUnitUnloaded.size();

	for (size_t i = 0; i < count; i++) {
		CEventClient* ec = listUnitUnloaded[i];
		const int ecAllyTeam = ec->GetReadAllyTeam();
		if (ec->GetFullRead() ||
		    (ecAllyTeam == unit->allyteam) ||
		    (ecAllyTeam == transport->allyteam)) {
			ec->UnitUnloaded(unit, transport);
		}
	}
}

inline void CEventHandler::FeatureCreated(const CFeature* feature)
{
	const int featureAllyTeam = feature->allyteam;
	const size_t count = listFeatureCreated.size();

	for (size_t i = 0; i < count; i++) {
		CEventClient* ec = listFeatureCreated[i];

		if ((featureAllyTeam < 0) || ec->CanReadAllyTeam(featureAllyTeam))
			ec->FeatureCreated(feature);
	}
}

inline void CEventHandler::FeatureDestroyed(const CFeature* feature)
{
	const int featureAllyTeam = feature->allyteam;
	const size_t count = listFeatureDestroyed.size();

	for (size_t i = 0; i < count; i++) {
		CEventClient* ec = listFeatureDestroyed[i];

		if ((featureAllyTeam < 0) || ec->CanReadAllyTeam(featureAllyTeam))
			ec->FeatureDestroyed(feature);
	}
}

inline void CEventHandler::FeatureDamaged(
	const CFeature* feature,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID)
{
	const int featureAllyTeam = feature->allyteam;
	const size_t count = listFeatureDamaged.size();

	for (size_t i = 0; i < count; i++) {
		CEventClient* ec = listFeatureDamaged[i];

		if (featureAllyTeam < 0 || ec->CanReadAllyTeam(featureAllyTeam))
			ec->FeatureDamaged(feature, attacker, damage, weaponDefID, projectileID);
	}
}

inline void CEventHandler::FeatureMoved(const CFeature* feature, const float3& oldpos)
{
	const int featureAllyTeam = feature->allyteam;
	const size_t count = listFeatureMoved.size();
	for (size_t i = 0; i < count; i++) {
		CEventClient* ec = listFeatureMoved[i];

		if ((featureAllyTeam < 0) || ec->CanReadAllyTeam(featureAllyTeam))
			ec->FeatureMoved(feature, oldpos);
	}
}


inline void CEventHandler::ProjectileCreated(const CProjectile* proj, int allyTeam)
{
	const size_t count = listProjectileCreated.size();
	for (size_t i = 0; i < count; i++) {
		CEventClient* ec = listProjectileCreated[i];
		if ((allyTeam < 0) || // projectile had no owner at creation
		    ec->CanReadAllyTeam(allyTeam)) {
			ec->ProjectileCreated(proj);
		}
	}
}


inline void CEventHandler::ProjectileDestroyed(const CProjectile* proj, int allyTeam)
{
	const size_t count = listProjectileDestroyed.size();

	for (size_t i = 0; i < count; i++) {
		CEventClient* ec = listProjectileDestroyed[i];
		if ((allyTeam < 0) || // projectile had no owner at creation
		    ec->CanReadAllyTeam(allyTeam)) {
			ec->ProjectileDestroyed(proj);
		}
	}
}


inline void CEventHandler::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	ITERATE_EVENTCLIENTLIST(UnsyncedHeightMapUpdate, rect)
}




inline bool CEventHandler::Explosion(int weaponDefID, int projectileID, const float3& pos, const CUnit* owner)
{
	auto& clients = listExplosion;

	for (size_t i = 0; i < clients.size(); ) {
		CEventClient* ec = clients[i];

		// discard return-value from clients lacking full-read access
		// (redundant for synced gadgets; watchWeaponDefs is checked)
		// NOTE: the call-in may remove itself from the client list
		if (!ec->Explosion(weaponDefID, projectileID, pos, owner) || !ec->GetFullRead()) {
			i += (i < clients.size() && ec == clients[i]);
			continue;
		}

		return true;
	}

	return false;
}


inline void CEventHandler::StockpileChanged(const CUnit* unit,
                                                const CWeapon* weapon,
                                                int oldCount)
{
	ITERATE_UNIT_ALLYTEAM_EVENTCLIENTLIST(StockpileChanged, unit, weapon, oldCount)
}


inline void CEventHandler::DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd)
{
	const size_t count = listDefaultCommand.size();

	for (size_t i = 0; i < count; i++) {
		CEventClient* ec = listDefaultCommand[i];

		ec->DefaultCommand(unit, feature, cmd);
	}
}



inline void CEventHandler::RenderUnitCreated(const CUnit* unit, int cloaked)
{
	ITERATE_EVENTCLIENTLIST(RenderUnitCreated, unit, cloaked)
}

UNIT_CALLIN_NO_PARAM(RenderUnitDestroyed)

inline void CEventHandler::RenderFeatureCreated(const CFeature* feature)
{
	ITERATE_EVENTCLIENTLIST(RenderFeatureCreated, feature)
}

inline void CEventHandler::RenderFeatureDestroyed(const CFeature* feature)
{
	ITERATE_EVENTCLIENTLIST(RenderFeatureDestroyed, feature)
}


inline void CEventHandler::RenderProjectileCreated(const CProjectile* proj)
{
	ITERATE_EVENTCLIENTLIST(RenderProjectileCreated, proj)
}

inline void CEventHandler::RenderProjectileDestroyed(const CProjectile* proj)
{
	ITERATE_EVENTCLIENTLIST(RenderProjectileDestroyed, proj)
}


#undef ITERATE_EVENTCLIENTLIST
#undef ITERATE_ALLYTEAM_EVENTCLIENTLIST
#undef ITERATE_UNIT_ALLYTEAM_EVENTCLIENTLIST
#undef UNIT_CALLIN_NO_PARAM
#undef UNIT_CALLIN_INT_PARAMS
#undef UNIT_CALLIN_LOS_PARAM

#endif /* EVENT_HANDLER_H */
