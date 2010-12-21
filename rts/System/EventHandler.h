/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <string>
#include <vector>
#include <map>

#include "EventClient.h"
#include "EventBatchHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/Projectile.h"

class CWeapon;
struct Command;
class CLogSubsystem;

class CEventHandler
{
	public:
		CEventHandler();
		~CEventHandler();

		void AddClient(CEventClient* ec);
		void RemoveClient(CEventClient* ec);

		bool InsertEvent(CEventClient* ec, const std::string& ciName);
		bool RemoveEvent(CEventClient* ec, const std::string& ciName);

		void GetEventList(std::vector<std::string>& list) const;

		bool IsKnown(const std::string& ciName) const;
		bool IsManaged(const std::string& ciName) const;
		bool IsUnsynced(const std::string& ciName) const;
		bool IsController(const std::string& ciName) const;

	public:
		// Synced events
		void Load(CArchiveBase* archive);

		void GamePreload();
		void GameStart();
		void GameOver( std::vector<unsigned char> winningAllyTeams );
		void GamePaused(int playerID, bool paused);
		void GameFrame(int gameFrame);

		void TeamDied(int teamID);
		void TeamChanged(int teamID);
		void PlayerChanged(int playerID);
		void PlayerAdded(int playerID);
		void PlayerRemoved(int playerID, int reason);

		void UnitCreated(const CUnit* unit, const CUnit* builder);
		void UnitFinished(const CUnit* unit);
		void UnitFromFactory(const CUnit* unit, const CUnit* factory,
		                     bool userOrders);
		void UnitDestroyed(const CUnit* unit, const CUnit* attacker);
		void UnitTaken(const CUnit* unit, int newTeam);
		void UnitGiven(const CUnit* unit, int oldTeam);

		void RenderUnitCreated(const CUnit* unit, int cloaked);
		void RenderUnitDestroyed(const CUnit* unit);
		void RenderUnitCloakChanged(const CUnit* unit, int cloaked);
		void RenderUnitLOSChanged(const CUnit* unit, int allyTeam, int newStatus);

		void DeleteSyncedUnits();
		void UpdateDrawUnits();
		void UpdateUnits();

		void UnitIdle(const CUnit* unit);
		void UnitCommand(const CUnit* unit, const Command& command);
		void UnitCmdDone(const CUnit* unit, int cmdType, int cmdTag);
		void UnitDamaged(const CUnit* unit, const CUnit* attacker,
		                 float damage, int weaponID, bool paralyzer);
		void UnitExperience(const CUnit* unit, float oldExperience);

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

		void UnitUnitCollision(const CUnit* collider, const CUnit* collidee);
		void UnitFeatureCollision(const CUnit* collider, const CFeature* collidee);
		void UnitMoveFailed(const CUnit* unit);

		void FeatureCreated(const CFeature* feature);
		void FeatureDestroyed(const CFeature* feature);
		void FeatureMoved(const CFeature* feature);

		void RenderFeatureCreated(const CFeature* feature);
		void RenderFeatureDestroyed(const CFeature* feature);
		void RenderFeatureMoved(const CFeature* feature);

		void DeleteSyncedFeatures();
		void UpdateDrawFeatures();
		void UpdateFeatures();

		void ProjectileCreated(const CProjectile* proj, int allyTeam);
		void ProjectileDestroyed(const CProjectile* proj, int allyTeam);

		void RenderProjectileCreated(const CProjectile* proj);
		void RenderProjectileDestroyed(const CProjectile* proj);

		void UnsyncedProjectileCreated(const CProjectile* proj);
		void UnsyncedProjectileDestroyed(const CProjectile* proj);

		void DeleteSyncedProjectiles();
		void UpdateDrawProjectiles();
		void UpdateProjectiles();

		void UpdateObjects();

		bool Explosion(int weaponID, const float3& pos, const CUnit* owner);

		void StockpileChanged(const CUnit* unit,
		                      const CWeapon* weapon, int oldCount);

	public:
		// Unsynced events
		void Save(zipFile archive);

		void Update();

		bool KeyPress(unsigned short key, bool isRepeat);
		bool KeyRelease(unsigned short key);
		bool MouseMove(int x, int y, int dx, int dy, int button);
		bool MousePress(int x, int y, int button);
		int  MouseRelease(int x, int y, int button); // return a cmd index, or -1
		bool MouseWheel(bool up, float value);
		bool JoystickEvent(const std::string& event, int val1, int val2);
		bool IsAbove(int x, int y);

		std::string GetTooltip(int x, int y);

		bool DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd);

		bool CommandNotify(const Command& cmd);

		bool AddConsoleLine(const std::string& msg, const CLogSubsystem& zone);

		bool GroupChanged(int groupID);

		bool GameSetup(const std::string& state, bool& ready,
		               const map<int, std::string>& playerStates);

		std::string WorldTooltip(const CUnit* unit,
		                         const CFeature* feature,
		                         const float3* groundPos);

		bool MapDrawCmd(int playerID, int type,
		                const float3* pos0,
		                const float3* pos1,
		                const std::string* label);

		void ViewResize();

		void DrawGenesis();
		void DrawWorld();
		void DrawWorldPreUnit();
		void DrawWorldShadow();
		void DrawWorldReflection();
		void DrawWorldRefraction();
		void DrawScreenEffects();
		void DrawScreen();
		void DrawInMiniMap();

		// FIXME: void ShockFront(float power, const float3& pos, float areaOfEffect);

	private:
		typedef vector<CEventClient*> EventClientList;

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

		typedef map<std::string, EventInfo> EventMap;

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

		// synced
		EventClientList listLoad;

		EventClientList listGamePreload;
		EventClientList listGameStart;
		EventClientList listGameOver;
		EventClientList listGamePaused;
		EventClientList listGameFrame;
		EventClientList listTeamDied;
		EventClientList listTeamChanged;
		EventClientList listPlayerChanged;
		EventClientList listPlayerAdded;
		EventClientList listPlayerRemoved;

		EventClientList listUnitCreated;
		EventClientList listUnitFinished;
		EventClientList listUnitFromFactory;
		EventClientList listUnitDestroyed;
		EventClientList listUnitTaken;
		EventClientList listUnitGiven;

		EventClientList listUnitIdle;
		EventClientList listUnitCommand;
		EventClientList listUnitCmdDone;
		EventClientList listUnitDamaged;
		EventClientList listUnitExperience;

		EventClientList listUnitSeismicPing;
		EventClientList listUnitEnteredRadar;
		EventClientList listUnitEnteredLos;
		EventClientList listUnitLeftRadar;
		EventClientList listUnitLeftLos;

		EventClientList listUnitEnteredWater;
		EventClientList listUnitEnteredAir;
		EventClientList listUnitLeftWater;
		EventClientList listUnitLeftAir;

		EventClientList listUnitLoaded;
		EventClientList listUnitUnloaded;

		EventClientList listUnitCloaked;
		EventClientList listUnitDecloaked;

		EventClientList listRenderUnitCreated;
		EventClientList listRenderUnitDestroyed;
		EventClientList listRenderUnitCloakChanged;
		EventClientList listRenderUnitLOSChanged;

		EventClientList listUnitUnitCollision;
		EventClientList listUnitFeatureCollision;
		EventClientList listUnitMoveFailed;

		EventClientList listFeatureCreated;
		EventClientList listFeatureDestroyed;
		EventClientList listFeatureMoved;

		EventClientList listRenderFeatureCreated;
		EventClientList listRenderFeatureDestroyed;
		EventClientList listRenderFeatureMoved;

		EventClientList listProjectileCreated;
		EventClientList listProjectileDestroyed;

		EventClientList listRenderProjectileCreated;
		EventClientList listRenderProjectileDestroyed;

		EventClientList listExplosion;

		EventClientList listStockpileChanged;

		// unsynced
		EventClientList listSave;

		EventClientList listUpdate;

		EventClientList listKeyPress;
		EventClientList listKeyRelease;
		EventClientList listMouseMove;
		EventClientList listMousePress;
		EventClientList listMouseRelease;
		EventClientList listMouseWheel;
		EventClientList listJoystickEvent;
		EventClientList listIsAbove;
		EventClientList listGetTooltip;

		EventClientList listDefaultCommand;
		EventClientList listConfigCommand;
		EventClientList listCommandNotify;
		EventClientList listAddConsoleLine;
		EventClientList listGroupChanged;
		EventClientList listGameSetup;
		EventClientList listWorldTooltip;
		EventClientList listMapDrawCmd;

		EventClientList listViewResize;

		EventClientList listDrawGenesis;
		EventClientList listDrawWorld;
		EventClientList listDrawWorldPreUnit;
		EventClientList listDrawWorldShadow;
		EventClientList listDrawWorldReflection;
		EventClientList listDrawWorldRefraction;
		EventClientList listDrawScreenEffects;
		EventClientList listDrawScreen;
		EventClientList listDrawInMiniMap;
};


extern CEventHandler eventHandler;


/******************************************************************************/
/******************************************************************************/
//
// Inlined call-in loops
//

inline void CEventHandler::UnitCreated(const CUnit* unit, const CUnit* builder)
{
//	(eventBatchHandler->GetUnitCreatedDestroyedBatch()).enqueue(unit);

	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitCreated.size();

	for (int i = 0; i < count; i++) {
		CEventClient* ec = listUnitCreated[i];
		if (ec->CanReadAllyTeam(unitAllyTeam)) {
			ec->UnitCreated(unit, builder);
		}
	}
}



#define UNIT_CALLIN_NO_PARAM(name)                                 \
	inline void CEventHandler:: name (const CUnit* unit)           \
	{                                                              \
		const int unitAllyTeam = unit->allyteam;                   \
		const int count = list ## name.size();                     \
		for (int i = 0; i < count; i++) {                          \
			CEventClient* ec = list ## name [i];                   \
			if (ec->CanReadAllyTeam(unitAllyTeam)) {               \
				ec-> name (unit);                                  \
			}                                                      \
		}                                                          \
	}

UNIT_CALLIN_NO_PARAM(UnitFinished)
UNIT_CALLIN_NO_PARAM(UnitIdle)
UNIT_CALLIN_NO_PARAM(UnitMoveFailed)
UNIT_CALLIN_NO_PARAM(UnitEnteredWater)
UNIT_CALLIN_NO_PARAM(UnitEnteredAir)
UNIT_CALLIN_NO_PARAM(UnitLeftWater)
UNIT_CALLIN_NO_PARAM(UnitLeftAir)



#define UNIT_CALLIN_INT_PARAM(name)                                       \
	inline void CEventHandler:: Unit ## name (const CUnit* unit, int p)   \
	{                                                                     \
		const int unitAllyTeam = unit->allyteam;                          \
		const int count = listUnit ## name.size();                        \
		for (int i = 0; i < count; i++) {                                 \
			CEventClient* ec = listUnit ## name [i];                      \
			if (ec->CanReadAllyTeam(unitAllyTeam)) {                      \
				ec-> Unit ## name (unit, p);                              \
			}                                                             \
		}                                                                 \
	}

UNIT_CALLIN_INT_PARAM(Taken)
UNIT_CALLIN_INT_PARAM(Given)



#define UNIT_CALLIN_LOS_PARAM(name)                                        \
	inline void CEventHandler:: Unit ## name (const CUnit* unit, int at)   \
	{                                                                      \
		eventBatchHandler->EnqueueUnitLOSStateChangeEvent(unit, at, unit->losStatus[at]);       \
		const int count = listUnit ## name.size();                         \
		for (int i = 0; i < count; i++) {                                  \
			CEventClient* ec = listUnit ## name [i];                       \
			if (ec->CanReadAllyTeam(at)) {                                 \
				ec-> Unit ## name (unit, at);                              \
			}                                                              \
		}                                                                  \
	}

UNIT_CALLIN_LOS_PARAM(EnteredRadar)
UNIT_CALLIN_LOS_PARAM(EnteredLos)
UNIT_CALLIN_LOS_PARAM(LeftRadar)
UNIT_CALLIN_LOS_PARAM(LeftLos)



inline void CEventHandler::UnitFromFactory(const CUnit* unit,
                                               const CUnit* factory,
                                               bool userOrders)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitFromFactory.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listUnitFromFactory[i];
		if (ec->CanReadAllyTeam(unitAllyTeam)) {
			ec->UnitFromFactory(unit, factory, userOrders);
		}
	}
}


inline void CEventHandler::UnitDestroyed(const CUnit* unit,
                                             const CUnit* attacker)
{
//	(eventBatchHandler->GetUnitCreatedDestroyedBatch()).dequeue_synced(unit);

	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitDestroyed.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listUnitDestroyed[i];
		if (ec->CanReadAllyTeam(unitAllyTeam)) {
			ec->UnitDestroyed(unit, attacker);
		}
	}
}

inline void CEventHandler::RenderUnitCreated(const CUnit* unit, int cloaked)
{
	const int count = listRenderUnitCreated.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listRenderUnitCreated[i];
		if (ec->CanReadAllyTeam(unit->allyteam)) {
			ec->RenderUnitCreated(unit, cloaked);
		}
	}
}

inline void CEventHandler::RenderUnitDestroyed(const CUnit* unit)
{
	const int count = listRenderUnitDestroyed.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listRenderUnitDestroyed[i];
		if (ec->CanReadAllyTeam(unit->allyteam)) {
			ec->RenderUnitDestroyed(unit);
		}
	}
}

inline void CEventHandler::RenderUnitCloakChanged(const CUnit* unit, int cloaked)
{
	const int count = listRenderUnitCloakChanged.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listRenderUnitCloakChanged[i];
		if (ec->CanReadAllyTeam(unit->allyteam)) {
			ec->RenderUnitCloakChanged(unit, cloaked);
		}
	}
}

inline void CEventHandler::RenderUnitLOSChanged(const CUnit* unit, int allyTeam, int newStatus)
{
	const int count = listRenderUnitLOSChanged.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listRenderUnitLOSChanged[i];
		if (ec->CanReadAllyTeam(unit->allyteam)) {
			ec->RenderUnitLOSChanged(unit, allyTeam, newStatus);
		}
	}
}

inline void CEventHandler::UnitCloaked(const CUnit* unit)
{
	eventBatchHandler->EnqueueUnitCloakStateChangeEvent(unit, 1);

	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitCloaked.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listUnitCloaked[i];
		if (ec->CanReadAllyTeam(unitAllyTeam)) {
			ec->UnitCloaked(unit);
		} 
	}
}

inline void CEventHandler::UnitDecloaked(const CUnit* unit)
{
	eventBatchHandler->EnqueueUnitCloakStateChangeEvent(unit, 0);

	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitDecloaked.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listUnitDecloaked[i];
		if (ec->CanReadAllyTeam(unitAllyTeam)) {
			ec->UnitDecloaked(unit);
		} 
	}
}



inline void CEventHandler::UnitUnitCollision(const CUnit* collider, const CUnit* collidee)
{
	const int colliderAllyTeam = collider->allyteam;
	const int clientCount = listUnitUnitCollision.size();

	for (int i = 0; i < clientCount; i++) {
		CEventClient* ec = listUnitUnitCollision[i];
		if (ec->CanReadAllyTeam(colliderAllyTeam)) {
			ec->UnitUnitCollision(collider, collidee);
		}
	}
}

inline void CEventHandler::UnitFeatureCollision(const CUnit* collider, const CFeature* collidee)
{
	const int colliderAllyTeam = collider->allyteam;
	const int clientCount = listUnitFeatureCollision.size();

	for (int i = 0; i < clientCount; i++) {
		CEventClient* ec = listUnitFeatureCollision[i];
		if (ec->CanReadAllyTeam(colliderAllyTeam)) {
			ec->UnitFeatureCollision(collider, collidee);
		}
	}
}



inline void CEventHandler::UpdateUnits(void) { eventBatchHandler->UpdateUnits(); }
inline void CEventHandler::UpdateDrawUnits() { eventBatchHandler->UpdateDrawUnits(); }
inline void CEventHandler::DeleteSyncedUnits() { eventBatchHandler->DeleteSyncedUnits(); }



inline void CEventHandler::UnitCommand(const CUnit* unit,
                                           const Command& command)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitCommand.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listUnitCommand[i];
		if (ec->CanReadAllyTeam(unitAllyTeam)) {
			ec->UnitCommand(unit, command);
		}
	}
}


inline void CEventHandler::UnitCmdDone(const CUnit* unit,
                                           int cmdID, int cmdTag)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitCmdDone.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listUnitCmdDone[i];
		if (ec->CanReadAllyTeam(unitAllyTeam)) {
			ec->UnitCmdDone(unit, cmdID, cmdTag);
		}
	}
}


inline void CEventHandler::UnitDamaged(const CUnit* unit,
                                           const CUnit* attacker,
                                           float damage, int weaponID,
                                           bool paralyzer)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitDamaged.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listUnitDamaged[i];
		if (ec->CanReadAllyTeam(unitAllyTeam)) {
			ec->UnitDamaged(unit, attacker, damage, weaponID, paralyzer);
		}
	}
}


inline void CEventHandler::UnitExperience(const CUnit* unit,
                                              float oldExperience)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitExperience.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listUnitExperience[i];
		if (ec->CanReadAllyTeam(unitAllyTeam)) {
			ec->UnitExperience(unit, oldExperience);
		}
	}
}


inline void  CEventHandler::UnitSeismicPing(const CUnit* unit,
                                                int allyTeam,
                                                const float3& pos,
                                                float strength)
{
	const int count = listUnitSeismicPing.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listUnitSeismicPing[i];
		if (ec->CanReadAllyTeam(allyTeam)) {
			ec->UnitSeismicPing(unit, allyTeam, pos, strength);
		}
	}
}


inline void CEventHandler::UnitLoaded(const CUnit* unit,
                                          const CUnit* transport)
{
	const int count = listUnitLoaded.size();
	for (int i = 0; i < count; i++) {
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
	const int count = listUnitUnloaded.size();
	for (int i = 0; i < count; i++) {
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
	(eventBatchHandler->GetFeatureCreatedDestroyedEventBatch()).enqueue(feature);

	const int featureAllyTeam = feature->allyteam;
	const int count = listFeatureCreated.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listFeatureCreated[i];
		if ((featureAllyTeam < 0) || // global team
		    ec->CanReadAllyTeam(featureAllyTeam)) {
			ec->FeatureCreated(feature);
		}
	}
}

inline void CEventHandler::FeatureDestroyed(const CFeature* feature)
{
	(eventBatchHandler->GetFeatureCreatedDestroyedEventBatch()).dequeue(feature);

	const int featureAllyTeam = feature->allyteam;
	const int count = listFeatureDestroyed.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listFeatureDestroyed[i];
		if ((featureAllyTeam < 0) || // global team
		    ec->CanReadAllyTeam(featureAllyTeam)) {
			ec->FeatureDestroyed(feature);
		}
	}
}

inline void CEventHandler::FeatureMoved(const CFeature* feature)
{
	(eventBatchHandler->GetFeatureMovedEventBatch()).enqueue(feature);

	const int featureAllyTeam = feature->allyteam;
	const int count = listFeatureMoved.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listFeatureMoved[i];
		if ((featureAllyTeam < 0) || // global team
		    ec->CanReadAllyTeam(featureAllyTeam)) {
			ec->FeatureMoved(feature);
		}
	}
}



inline void CEventHandler::RenderFeatureCreated(const CFeature* feature)
{
	const int count = listRenderFeatureCreated.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listRenderFeatureCreated[i];
		ec->RenderFeatureCreated(feature);
	}
}

inline void CEventHandler::RenderFeatureDestroyed(const CFeature* feature)
{
	const int count = listRenderFeatureDestroyed.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listRenderFeatureDestroyed[i];
		ec->RenderFeatureDestroyed(feature);
	}
}

inline void CEventHandler::RenderFeatureMoved(const CFeature* feature)
{
	const int count = listRenderFeatureMoved.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listRenderFeatureMoved[i];
		ec->RenderFeatureMoved(feature);
	}
}



inline void CEventHandler::UpdateFeatures(void) { eventBatchHandler->UpdateFeatures(); }
inline void CEventHandler::UpdateDrawFeatures() { eventBatchHandler->UpdateDrawFeatures(); }
inline void CEventHandler::DeleteSyncedFeatures() { eventBatchHandler->DeleteSyncedFeatures(); }



inline void CEventHandler::ProjectileCreated(const CProjectile* proj, int allyTeam)
{
	if (proj->synced) {
		(eventBatchHandler->GetSyncedProjectileCreatedDestroyedBatch()).insert(proj);
	} else {
		(eventBatchHandler->GetUnsyncedProjectileCreatedDestroyedBatch()).insert(proj);
	}

	const int count = listProjectileCreated.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listProjectileCreated[i];
		if ((allyTeam < 0) || // projectile had no owner at creation
		    ec->CanReadAllyTeam(allyTeam)) {
			ec->ProjectileCreated(proj);
		}
	}
}


inline void CEventHandler::ProjectileDestroyed(const CProjectile* proj, int allyTeam)
{
	if (proj->synced) {
		(eventBatchHandler->GetSyncedProjectileCreatedDestroyedBatch()).erase_remove_synced(proj);
	} else {
		(eventBatchHandler->GetUnsyncedProjectileCreatedDestroyedBatch()).erase_delete(proj);
	}

	const int count = listProjectileDestroyed.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listProjectileDestroyed[i];
		if ((allyTeam < 0) || // projectile had no owner at creation
		    ec->CanReadAllyTeam(allyTeam)) {
			ec->ProjectileDestroyed(proj);
		}
	}
}


inline void CEventHandler::UnsyncedProjectileCreated(const CProjectile* proj) {
	(eventBatchHandler->GetUnsyncedProjectileCreatedDestroyedBatch()).insert(proj);
}

inline void CEventHandler::UnsyncedProjectileDestroyed(const CProjectile* proj) {
	(eventBatchHandler->GetUnsyncedProjectileCreatedDestroyedBatch()).erase_delete(proj);
}


inline void CEventHandler::RenderProjectileCreated(const CProjectile* proj)
{
	const int count = listRenderProjectileCreated.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listRenderProjectileCreated[i];
		ec->RenderProjectileCreated(proj);
	}
}

inline void CEventHandler::RenderProjectileDestroyed(const CProjectile* proj)
{
	const int count = listRenderProjectileDestroyed.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listRenderProjectileDestroyed[i];
		ec->RenderProjectileDestroyed(proj);
	}
}



inline void CEventHandler::UpdateProjectiles() { eventBatchHandler->UpdateProjectiles(); }
inline void CEventHandler::UpdateDrawProjectiles() { eventBatchHandler->UpdateDrawProjectiles(); }
inline void CEventHandler::DeleteSyncedProjectiles() { eventBatchHandler->DeleteSyncedProjectiles(); }

inline void CEventHandler::UpdateObjects() {
	eventBatchHandler->UpdateObjects();
}

inline bool CEventHandler::Explosion(int weaponID, const float3& pos, const CUnit* owner)
{
	const int count = listExplosion.size();
	bool noGfx = false;
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listExplosion[i];
		if (ec->GetFullRead()) {
			noGfx = noGfx || ec->Explosion(weaponID, pos, owner);
		}
	}
	return noGfx;
}


inline void CEventHandler::StockpileChanged(const CUnit* unit,
                                                const CWeapon* weapon,
                                                int oldCount)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listStockpileChanged.size();
	for (int i = 0; i < count; i++) {
		CEventClient* ec = listStockpileChanged[i];
		//const int ecAllyTeam = ec->GetReadAllyTeam();
		if (ec->CanReadAllyTeam(unitAllyTeam)) {
			ec->StockpileChanged(unit, weapon, oldCount);
		}
	}
}


inline bool CEventHandler::DefaultCommand(const CUnit* unit,
                                              const CFeature* feature,
                                              int& cmd)
{
	// reverse order, user has the override
	const int count = listDefaultCommand.size();
	for (int i = (count - 1); i >= 0; i--) {
		CEventClient* ec = listDefaultCommand[i];
		if (ec->DefaultCommand(unit, feature, cmd)) {
			return true;
		}
	}
	return false;
}


#endif /* EVENT_HANDLER_H */
