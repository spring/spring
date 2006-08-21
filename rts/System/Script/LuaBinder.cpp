#include "StdAfx.h"
#include "LuaBinder.h"
#include "Game/StartScripts/Script.h"
#include "float3.h"
#include "Game/UI/InfoConsole.h"
#include "GlobalStuff.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "TdfParser.h"
#include "LuaFunctions.h"
#include "Game/command.h"
#include "Sim/Units/UnitDef.h"

extern "C"
{
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
}

#include <luabind/luabind.hpp>
#include <luabind/discard_result_policy.hpp>
#include <luabind/adopt_policy.hpp>

#include <boost/shared_ptr.hpp>

using namespace std;
using namespace luabind;
using namespace luafunctions;

extern std::string stupidGlobalMapname;

// Simple error message handling
#define check(a) { try { a } catch(luabind::error& e) { ShowLuaError(e.state()); } }
void ShowLuaError(lua_State* l)
{
	info->AddLine("Lua error: %s", lua_tostring(l, -1));
}

// Wrapper class to allow lua classes to create new startscripts by subclassing Script
struct CScript_wrapper : CScript, wrap_base
{
	CScript_wrapper(const string& name) : CScript(name) {}

	virtual void Update() { check( call<void>("Update"); );	}
	static void Update_static(CScript *ptr) { ptr->CScript::Update(); }
	virtual string GetMapName() { string x; check( x = call<string>("GetMapName"); ); return x; }
	static string GetMapName_static(CScript *ptr) { return ptr->CScript::GetMapName(); }
	virtual void GotChatMsg(const string& msg, int player) { check( call<void>("GotChatMsg", msg, player); ); }
	static void GotChatMsg_static(CScript *ptr, const string& msg, int player) { ptr->CScript::GotChatMsg(msg, player); }
};

namespace luabind 
{
	template<class T>
	T* get_pointer(CObject_pointer<T>& p) 
	{
		if (!p.held)
			info->AddLine("Lua warning: using invalid unit reference");
		return p.held; 
	}

	template<class A>
	CObject_pointer<const A>* get_const_holder(CObject_pointer<A>*)
	{
		return 0;
	}
}

namespace luafunctions {

	bool UnitPointerIsValid(CObject_pointer<CUnit>* u)
	{
		return (u->held != NULL);
	}

	// Some useful helper methods
	string FloatToString(float3 f)
	{
		char tmp[100];
		sprintf(tmp, "%.1f %.1f %.1f", f.x, f.y, f.z);
		return string(tmp);
	}

	string UnitToString(CUnit* u)
	{
		char tmp[100];
		sprintf(tmp, "unit %d", u->id);
		return string(tmp);
	}

	int TdfGetInteger(TdfParser* s, int def, string key)
	{
		int val;
		char buf[100];
		sprintf(buf, "%d", def);
		s->GetDef(val, buf, key);
		return val;
	}

	float TdfGetFloat(TdfParser* s, float def, string key)
	{
		float val;
		char buf[100];
		sprintf(buf, "%f", def);
		s->GetDef(val, buf, key);
		return val;
	}

	string TdfGetString(TdfParser* s, string def, string key)
	{
		return s->SGetValueDef(def, key);
	}

	string GSGetMapName()
	{
		return stupidGlobalMapname.substr(0,stupidGlobalMapname.find_last_of('.'));
	}

	void DisabledFunction()
	{
		info->AddLine("Lua: Attempt to call disabled function");
	}

	// Redefinition of the lua standard function print to use the spring infoconsole
	int SpringPrint (lua_State *L) {
		int n = lua_gettop(L);  /* number of arguments */
		int i;
		lua_getglobal(L, "tostring");
		string tmp;

		for (i=1; i<=n; i++) {
			const char *s;
			lua_pushvalue(L, -1);  /* function to be called */
			lua_pushvalue(L, i);   /* value to print */
			lua_call(L, 1, 1);
			s = lua_tostring(L, -1);  /* get result */
			if (s == NULL)
				return luaL_error(L, "`tostring' must return a string to `print'");
    
			if (i > 1)
				tmp += " ";
			tmp += string(s);
			//if (i>1) fputs("\t", stdout);    
			//fputs(s, stdout);
			lua_pop(L, 1);  /* pop result */
		}

		if (info)
			info->AddLine(tmp.c_str());

		return 0;
	}

}

CLuaBinder::CLuaBinder(void)
{
	// Create a new lua state
	luaState = lua_open();

	// Connect LuaBind to this lua state
	open(luaState);

	// Setup the lua global environment
	luaopen_base(luaState);
	//luaopen_io(luaState);
	luaopen_string(luaState);
	luaopen_table(luaState);
	luaopen_math(luaState);

	// Define the useful spring classes to lua
	module(luaState)
	[
		class_<CScript, CScript_wrapper>("Script")
			.def(constructor<const string&>())
			.def_readwrite("onlySinglePlayer", &CScript::onlySinglePlayer)
			.def("GetMapName", &CScript::GetMapName, &CScript_wrapper::GetMapName_static)
			.def("Update", &CScript::Update, &CScript_wrapper::Update_static)
			.def("GotChatMsg", &CScript::GotChatMsg, &CScript_wrapper::GotChatMsg_static),

		class_<CGlobalSyncedStuff>("GlobalSynced")
			.def_readonly("frameNum", &CGlobalSyncedStuff::frameNum)
			.def_readonly("mapx", &CGlobalSyncedStuff::mapx)
			.def_readonly("mapy", &CGlobalSyncedStuff::mapy),

		class_<float3>("float3")
			.def(constructor<const float, const float, const float>())
			.def_readwrite("x", &float3::x)
			.def_readwrite("y", &float3::y)
			.def_readwrite("z", &float3::z)
			.def("__tostring", &FloatToString),

		class_<CWorldObject>("WorldObject")
			.def_readonly("pos", &CWorldObject::pos), 

		class_<CUnit, bases<CWorldObject>, CObject_pointer<CUnit> >("Unit")
			.enum_("constants")
			[
				value("GIVEN", CUnit::ChangeGiven),
				value("CAPTURED", CUnit::ChangeCaptured)
			]
			.def_readonly("id", &CUnit::id)
			.def_readonly("health", &CUnit::health)
			.property("transporter", &UnitGetTransporter)
			.def_readonly("definition", &CUnit::unitDef)
			.def("__tostring", &UnitToString)
			.def_readonly("team", &CUnit::team)
			.def("GiveCommand", &UnitGiveCommand) 
			.def("ChangeTeam", &CUnit::ChangeTeam)
			.def("IsValid", &UnitPointerIsValid),

		class_<UnitDef>("UnitDef")
			.def_readonly("name", &UnitDef::name),

		class_<TdfParser>("TdfParser")
			.def(constructor<>())
			.def("LoadFile", &TdfParser::LoadFile)
			.def("GetInteger", &TdfGetInteger)
			.def("GetFloat", &TdfGetFloat)
			.def("GetString", &TdfGetString),

		class_<Command>("Command")
			.enum_("constants")
			[
				value("STOP", CMD_STOP),
				value("WAIT", CMD_WAIT),
				value("MOVE", CMD_MOVE),
				value("PATROL", CMD_PATROL),
				value("ATTACK", CMD_ATTACK),
				value("AREA_ATTACK", CMD_AREA_ATTACK),
				value("GUARD", CMD_GUARD),
				value("LOAD_UNITS", CMD_LOAD_UNITS),
				value("UNLOAD_UNIT", CMD_UNLOAD_UNIT)
			]
			.def(constructor<>())
			.def_readwrite("id", &Command::id)
			.def_readwrite("options", &Command::options)
			.def("AddParam", &CommandAddParam),

		// Access to spring's various global handlers are grouped into 
		// relevant lua namespaces to present a nice(r) interface than just exporting
		// all the handlers directly
		namespace_("units")
		[
			def("Load", &UnitLoaderLoadUnit, adopt(result)),
			def("GetNumAt", &GetNumUnitsAt),
			def("GetAt", &GetUnitsAt, raw(_1)),
			def("GetSelected", &GetSelectedUnits, raw(_1)),
			def("SendSelection", &SendSelectedUnits)
		],

		namespace_("map")
		[
			def("GetName", &GSGetMapName),
			def("GetTDFName", &MapGetTDFName)
		],

		def("EndGame", &EndGame),
		
		// File access should probably be limited to the virtual filesystem. Disabled for now
		def("dofile", &DisabledFunction),
		def("loadfile", &DisabledFunction),
		def("loadlib", &DisabledFunction),
		def("require", &DisabledFunction),

		// Random number usage must go through spring functions in a synced script
		namespace_("math")
		[
			def("random", &DisabledFunction)
		]
	]; 

	// Define global objects
	globals(luaState)["gs"] = gs;

	// Special override
	lua_register(luaState, "print", SpringPrint);

	lastError = "";
}

bool CLuaBinder::LoadScript(const string& name)
{
	if (luaL_loadfile(luaState, name.c_str()) || lua_pcall(luaState, 0, 0, 0)) {
		lastError = lua_tostring(luaState, -1);
		return false;
	}

	return true;
}

CLuaBinder::~CLuaBinder(void)
{
	lua_close(luaState);
}
