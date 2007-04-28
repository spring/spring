#ifndef LUA_CALL_IN_HANDLER_H
#define LUA_CALL_IN_HANDLER_H
// LuaCallInHandler.h: interface for the CLuaCallInHandler class.
//
// NOTE: this should probably be merged with GlobalAIHandler.h,
//       make a base class for both GlobalAI and LuaHandle
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "LuaHandle.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/Feature.h"


class CLuaCallInHandler
{
	public:
		CLuaCallInHandler();
		~CLuaCallInHandler();

		void AddHandle(CLuaHandle* lh);
		void RemoveHandle(CLuaHandle* lh);

	public:
		void Update();

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
		void UnitDamaged(const CUnit* unit, const CUnit* attacker, float damage);

		void UnitSeismicPing(const CUnit* unit, int allyTeam,
		                     const float3& pos, float strength);
		void UnitEnteredRadar(const CUnit* unit, int allyTeam);
		void UnitEnteredLos(const CUnit* unit, int allyTeam);
		void UnitLeftRadar(const CUnit* unit, int allyTeam);
		void UnitLeftLos(const CUnit* unit, int allyTeam);

		void UnitLoaded(const CUnit* unit, const CUnit* transport);
		void UnitUnloaded(const CUnit* unit, const CUnit* transport);

		void FeatureCreated(const CFeature* feature);
		void FeatureDestroyed(const CFeature* feature);

		void DrawWorld();
		void DrawWorldShadow();
		void DrawWorldReflection();
		void DrawWorldRefraction();
		void DrawScreen();
		void DrawInMiniMap();

	private:
		typedef vector<CLuaHandle*> CallInList;

	private:
		void ListInsert(CallInList& ciList, CLuaHandle* lh);
		void ListRemove(CallInList& ciList, CLuaHandle* lh);

	private:
		CallInList handles;

		CallInList listUpdate;

		CallInList listGameOver;
		CallInList listTeamDied;

		CallInList listUnitCreated;
		CallInList listUnitFinished;
		CallInList listUnitFromFactory;
		CallInList listUnitDestroyed;
		CallInList listUnitTaken;
		CallInList listUnitGiven;

		CallInList listUnitIdle;
		CallInList listUnitDamaged;

		CallInList listUnitSeismicPing;
		CallInList listUnitEnteredRadar;
		CallInList listUnitEnteredLos;
		CallInList listUnitLeftRadar;
		CallInList listUnitLeftLos;

		CallInList listUnitLoaded;
		CallInList listUnitUnloaded;

		CallInList listFeatureCreated;
		CallInList listFeatureDestroyed;

		CallInList listDrawWorld;
		CallInList listDrawWorldShadow;
		CallInList listDrawWorldReflection;
		CallInList listDrawWorldRefraction;
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


inline void CLuaCallInHandler::UnitDamaged(const CUnit* unit,
                                           const CUnit* attacker,
                                           float damage)
{
	const int unitAllyTeam = unit->allyteam;
	const int count = listUnitDamaged.size();
	for (int i = 0; i < count; i++) {
		CLuaHandle* lh = listUnitDamaged[i];
		if (lh->GetFullRead() || (lh->GetReadAllyTeam() == unitAllyTeam)) {
			lh->UnitDamaged(unit, attacker, damage);
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


#endif /* LUA_CALL_IN_HANDLER_H */
