#ifndef __LUA_BINDER
#define __LUA_BINDER

#include <string>

struct lua_State;

class CLuaBinder
{
protected:
	lua_State* luaState;
public:
	std::string lastError;
	CLuaBinder(void);
	bool LoadScript(const std::string& name, char* buffer, int size);
	~CLuaBinder(void);
};

extern void ShowLuaError(lua_State* l);

#endif

