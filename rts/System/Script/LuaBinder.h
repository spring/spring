#ifndef __LUA_BINDER
#define __LUA_BINDER

#include <string>
#include "System/Object.h"

struct lua_State;
class CUnit;

class CLuaBinder
{
protected:
	lua_State* luaState;
public:
	std::string lastError;
	CLuaBinder(void);
	void CreateLateBindings();
	bool LoadScript(const std::string& name);
	~CLuaBinder(void);
};

// Scripts can't have direct access to CUnit instances since they may become invalid at any time
class CUnit_pointer : public CObject
{
public:
	CUnit *unit;
	CUnit_pointer() : unit(NULL) {}
	CUnit_pointer(CUnit* u);
	~CUnit_pointer();
	virtual void DependentDied(CObject* o);
};

void ShowLuaError(lua_State* l);

#endif

