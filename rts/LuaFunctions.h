#ifndef __LUA_FUNCTIONS
#define __LUA_FUNCTIONS

#include "LuaBinder.h"

struct Command;
class float3;

namespace luafunctions {

	void EndGame();
	void UnitGiveCommand(CUnit_pointer* u, Command* c);
	void CommandAddParam(Command* c, float p);
	CUnit_pointer* UnitLoaderLoadUnit(std::string name, float3 pos, int team, bool buil);
	int GetNumUnitsAt(const float3& pos, float radius);

};

#endif

