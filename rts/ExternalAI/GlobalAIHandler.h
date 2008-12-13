#ifndef GLOBALAIHANDLER_H
#define GLOBALAIHANDLER_H

#include "ConfigHandler.h"
#include "float3.h"
#include "Sim/Misc/GlobalConstants.h"
#include <map>
#include <string>

class CUnit;
class CGlobalAI;
struct WeaponDef;
struct Command;

class CGlobalAIHandler
{
public:
	CR_DECLARE(CGlobalAIHandler);
//	CR_DECLARE_SUB(AIMemBuffer);
	static bool CatchException();

	CGlobalAIHandler(void);
	~CGlobalAIHandler(void);
	void PostLoad();

	void Update(void);
	void PreDestroy ();

	void UnitEnteredLos(CUnit* unit,int allyteam);
	void UnitLeftLos(CUnit* unit,int allyteam);
	void UnitEnteredRadar(CUnit* unit,int allyteam);
	void UnitLeftRadar(CUnit* unit,int allyteam);
	void SeismicPing(int allyteam, CUnit *unit, const float3 &pos, float strength);

	void UnitIdle(CUnit* unit);
	void UnitCreated(CUnit* unit);
	void UnitFinished(CUnit* unit);
	void UnitDestroyed(CUnit* unit, CUnit *attacker);
	void UnitDamaged(CUnit* attacked,CUnit* attacker,float damage);
	void UnitMoveFailed(CUnit* unit);
	void UnitTaken(CUnit* unit, int newteam);
	void UnitGiven(CUnit* unit, int oldteam);
	void WeaponFired(CUnit* unit, const WeaponDef* def);
	void PlayerCommandGiven(std::vector<int>& selectedunits,Command& c,int player);
	void Load(std::istream *s);
	void Save(std::ostream *s);
	CGlobalAI* ais[MAX_TEAMS];
	bool hasAI;

	struct AIMemBuffer{
//		CR_DECLARE_STRUCT(AIMemBuffer);
		char* mem;
		int usage;
	};
	std::map<std::string,AIMemBuffer> memBuffers[MAX_TEAMS];

	bool CreateGlobalAI(int team, const char* dll);
	void* GetAIBuffer(int team, std::string name, int length);
	void ReleaseAIBuffer(int team, std::string name);
	void GotChatMsg(const char* msg, int player);
};

extern CGlobalAIHandler* globalAI;

#endif
