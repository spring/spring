#ifndef LUA_CALL_IN_HANDLER_H
#define LUA_CALL_IN_HANDLER_H
// LuaCallInHandler.h: interface for the CLuaCallInHandler class.
//
// NOTE: this should probably be merged with GlobalAIHandler.h,
//       make a base class for both GlobalAI and LuaHandle
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <map>
using std::string;
using std::vector;
using std::map;

#include "LuaHandle.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"


class CUnit;
class CWeapon;


class CLuaCallInHandler
{
	public:
		CLuaCallInHandler();
		~CLuaCallInHandler();

		void AddHandle(CLuaHandle* lh);
		void RemoveHandle(CLuaHandle* lh);

		bool ManagedCallIn(const string& ciName);
		bool UnsyncedCallIn(const string& ciName);

		bool InsertCallIn(CLuaHandle* lh, const string& ciName);
		bool RemoveCallIn(CLuaHandle* lh, const string& ciName);

	public:
		// Synced
		void GameLoadLua();
		void GameStartPlaying();
		void GameOver();
		void TeamDied(int teamID);

		void PlayerRemoved(int playerID); // not implemented

		void UnitCreated(const CUnit* unit, const CUnit* builder);
		void UnitFinished(const CUnit* unit);
		void UnitFromFactory(const CUnit* unit, const CUnit* factory,
		                     bool userOrders);
		void UnitDestroyed(const CUnit* unit, const CUnit* attacker);
		void UnitTaken(const CUnit* unit, int newTeam);
		void UnitGiven(const CUnit* unit, int oldTeam);

		void UnitIdle(const CUnit* unit);
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

		void UnitLoaded(const CUnit* unit, const CUnit* transport);
		void UnitUnloaded(const CUnit* unit, const CUnit* transport);

		void UnitCloaked(const CUnit* unit);
		void UnitDecloaked(const CUnit* unit);

		void FeatureCreated(const CFeature* feature);
		void FeatureDestroyed(const CFeature* feature);

		void StockpileChanged(const CUnit* unit,
		                      const CWeapon* weapon, int oldCount);
	
		bool Explosion(int weaponID, const float3& pos, const CUnit* owner);

		// Unsynced
		void Update();

		bool DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd);

		void DrawGenesis();
		void DrawWorld();
		void DrawWorldPreUnit();
		void DrawWorldShadow();
		void DrawWorldReflection();
		void DrawWorldRefraction();
		void DrawScreenEffects();
		void DrawScreen();
		void DrawInMiniMap();

	private:
		typedef vector<CLuaHandle*> CallInList;

	private:
		void ListInsert(CallInList& ciList, CLuaHandle* lh);
		void ListRemove(CallInList& ciList, CLuaHandle* lh);

	private:
		map<string, CallInList*> callInMap;

		CallInList handles;

		CallInList listGameLoadLua;
		CallInList listGameStartPlaying;
		CallInList listGameOver;
		CallInList listTeamDied;

		CallInList listUnitCreated;
		CallInList listUnitFinished;
		CallInList listUnitFromFactory;
		CallInList listUnitDestroyed;
		CallInList listUnitTaken;
		CallInList listUnitGiven;

		CallInList listUnitIdle;
		CallInList listUnitCmdDone;
		CallInList listUnitDamaged;
		CallInList listUnitExperience;

		CallInList listUnitSeismicPing;
		CallInList listUnitEnteredRadar;
		CallInList listUnitEnteredLos;
		CallInList listUnitLeftRadar;
		CallInList listUnitLeftLos;

		CallInList listUnitLoaded;
		CallInList listUnitUnloaded;

		CallInList listUnitCloaked;
		CallInList listUnitDecloaked;

		CallInList listFeatureCreated;
		CallInList listFeatureDestroyed;

		CallInList listStockpileChanged;

		CallInList listExplosion;

		CallInList listUpdate;

		CallInList listDefaultCommand;

		CallInList listDrawGenesis;
		CallInList listDrawWorld;
		CallInList listDrawWorldPreUnit;
		CallInList listDrawWorldShadow;
		CallInList listDrawWorldReflection;
		CallInList listDrawWorldRefraction;
		CallInList listDrawScreenEffects;
		CallInList listDrawScreen;
		CallInList listDrawInMiniMap;
};


extern CLuaCallInHandler luaCallIns;


//
// Inlined call-in loops
//

inline void CLuaCallInHandler::UnitCreated(const CUnit* unit,
                                           const CUnit* builder)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitCreated.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listUnitCreated[i];
		if (lh->GetFullRead() || (lh->GetReadAllyTeam() == unitAllyTeam)) {
			lh->UnitCreated(unit, builder);
		}
	}
}


#define UNIT_CALLIN_NO_PARAM(name)                                        \
	inline void CLuaCallInHandler:: Unit ## name (const CUnit* unit)        \
	{                                                                       \
		const int unitAllyTeam = unit->allyteam;                              \
		const int count = listUnit ## name.size();                            \
		for (int i = 0; i < count; i++) {                                     \
			CLuaHandle* lh = listUnit ## name [i];                              \
			if (lh->GetFullRead() || (lh->GetReadAllyTeam() == unitAllyTeam)) { \
				lh-> Unit ## name (unit);                                         \
			}                                                                   \
		}                                                                     \
	}

UNIT_CALLIN_NO_PARAM(Finished)
UNIT_CALLIN_NO_PARAM(Idle)
UNIT_CALLIN_NO_PARAM(Cloaked)
UNIT_CALLIN_NO_PARAM(Decloaked)


#define UNIT_CALLIN_INT_PARAM(name)                                       \
	inline void CLuaCallInHandler:: Unit ## name (const CUnit* unit, int p) \
	{                                                                       \
		const int unitAllyTeam = unit->allyteam;                              \
		const int count = listUnit ## name.size();                            \
		for (int i = 0; i < count; i++) {                                     \
			CLuaHandle* lh = listUnit ## name [i];                              \
			if (lh->GetFullRead() || (lh->GetReadAllyTeam() == unitAllyTeam)) { \
				lh-> Unit ## name (unit, p);                                      \
			}                                                                   \
		}                                                                     \
	}

UNIT_CALLIN_INT_PARAM(Taken)
UNIT_CALLIN_INT_PARAM(Given)


#define UNIT_CALLIN_LOS_PARAM(name)                                        \
	inline void CLuaCallInHandler:: Unit ## name (const CUnit* unit, int at) \
	{                                                                        \
		const int count = listUnit ## name.size();                             \
		for (int i = 0; i < count; i++) {                                      \
			CLuaHandle* lh = listUnit ## name [i];                               \
			if (lh->GetFullRead() || (lh->GetReadAllyTeam() == at)) {            \
				lh-> Unit ## name (unit, at);                                      \
			}                                                                    \
		}                                                                      \
	}

UNIT_CALLIN_LOS_PARAM(EnteredRadar)
UNIT_CALLIN_LOS_PARAM(EnteredLos)
UNIT_CALLIN_LOS_PARAM(LeftRadar)
UNIT_CALLIN_LOS_PARAM(LeftLos)


inline void CLuaCallInHandler::UnitFromFactory(const CUnit* unit,
                                               const CUnit* factory,
                                               bool userOrders)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitFromFactory.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listUnitFromFactory[i];
		if (lh->GetFullRead() || (lh->GetReadAllyTeam() == unitAllyTeam)) {
			lh->UnitFromFactory(unit, factory, userOrders);
		}
	}
}


inline void CLuaCallInHandler::UnitDestroyed(const CUnit* unit,
                                             const CUnit* attacker)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitDestroyed.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listUnitDestroyed[i];
		if (lh->GetFullRead() || (lh->GetReadAllyTeam() == unitAllyTeam)) {
			lh->UnitDestroyed(unit, attacker);
		}
	}
}


inline void CLuaCallInHandler::UnitCmdDone(const CUnit* unit,
                                           int cmdID, int cmdTag)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitCmdDone.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listUnitCmdDone[i];
		if (lh->GetFullRead() || (lh->GetReadAllyTeam() == unitAllyTeam)) {
			lh->UnitCmdDone(unit, cmdID, cmdTag);
		}
	}
}


inline void CLuaCallInHandler::UnitDamaged(const CUnit* unit,
                                           const CUnit* attacker,
                                           float damage, int weaponID,
                                           bool paralyzer)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitDamaged.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listUnitDamaged[i];
		if (lh->GetFullRead() || (lh->GetReadAllyTeam() == unitAllyTeam)) {
			lh->UnitDamaged(unit, attacker, damage, weaponID, paralyzer);
		}
	}
}


inline void CLuaCallInHandler::UnitExperience(const CUnit* unit,
                                              float oldExperience)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitExperience.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listUnitExperience[i];
		if (lh->GetFullRead() || (lh->GetReadAllyTeam() == unitAllyTeam)) {
			lh->UnitExperience(unit, oldExperience);
		}
	}
}


inline void  CLuaCallInHandler::UnitSeismicPing(const CUnit* unit,
                                                int allyTeam,
                                                const float3& pos,
                                                float strength)
{
	const int count = listUnitSeismicPing.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listUnitSeismicPing[i];
		if (lh->GetFullRead() || (lh->GetReadAllyTeam() == allyTeam)) {
			lh->UnitSeismicPing(unit, allyTeam, pos, strength);
		}
	}
}


inline void CLuaCallInHandler::UnitLoaded(const CUnit* unit,
                                          const CUnit* transport)
{
	const int count = listUnitLoaded.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listUnitLoaded[i];
		const int lhAllyTeam = lh->GetReadAllyTeam();
		if (lh->GetFullRead() ||
		    (lhAllyTeam == unit->allyteam) ||
		    (lhAllyTeam == transport->allyteam)) {
			lh->UnitLoaded(unit, transport);
		}
	}
}


inline void CLuaCallInHandler::UnitUnloaded(const CUnit* unit,
                                            const CUnit* transport)
{
	const int count = listUnitUnloaded.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listUnitUnloaded[i];
		const int lhAllyTeam = lh->GetReadAllyTeam();
		if (lh->GetFullRead() ||
		    (lhAllyTeam == unit->allyteam) ||
		    (lhAllyTeam == transport->allyteam)) {
			lh->UnitUnloaded(unit, transport);
		}
	}
}


inline void CLuaCallInHandler::FeatureCreated(const CFeature* feature)
{
	const int featureAllyTeam = feature->allyteam;
	const int count = listFeatureCreated.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listFeatureCreated[i];
		if ((featureAllyTeam < 0) || // global team
		    lh->GetFullRead() || (lh->GetReadAllyTeam() == featureAllyTeam)) {
			lh->FeatureCreated(feature);
		}
	}
}


inline void CLuaCallInHandler::FeatureDestroyed(const CFeature* feature)
{
	const int featureAllyTeam = feature->allyteam;
	const int count = listFeatureDestroyed.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listFeatureDestroyed[i];
		if ((featureAllyTeam < 0) || // global team
		    lh->GetFullRead() || (lh->GetReadAllyTeam() == featureAllyTeam)) {
			lh->FeatureDestroyed(feature);
		}
	}
}


inline void CLuaCallInHandler::StockpileChanged(const CUnit* unit,
                                                const CWeapon* weapon,
                                                int oldCount)
{
	const int count = listStockpileChanged.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listStockpileChanged[i];
		const int lhAllyTeam = lh->GetReadAllyTeam();
		if (lh->GetFullRead() || (lhAllyTeam == unit->allyteam)) {
			lh->StockpileChanged(unit, weapon, oldCount);
		}
	}
}


inline bool CLuaCallInHandler::Explosion(int weaponID,
                                         const float3& pos, const CUnit* owner)
{
	const int count = listExplosion.size();
	bool noGfx = false;
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listExplosion[i];
		if (lh->GetFullRead()) {
			noGfx = noGfx || lh->Explosion(weaponID, pos, owner);
		}
	}
	return noGfx;
}


inline bool CLuaCallInHandler::DefaultCommand(const CUnit* unit,
                                              const CFeature* feature,
                                              int& cmd)
{
	// reverse order, user has the override
	const int count = listDefaultCommand.size();
	for (int i = (count - 1); i >= 0; i--) {
		CLuaHandle* lh = listDefaultCommand[i];
		if (lh->DefaultCommand(unit, feature, cmd)) {
			return true;
		}
	}
	return false;
}


#endif /* LUA_CALL_IN_HANDLER_H */
