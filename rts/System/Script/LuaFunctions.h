#ifndef __LUA_FUNCTIONS
#define __LUA_FUNCTIONS

#include <luabind/luabind.hpp>

#include "LuaBinder.h"
#include "Object.h"

#include <map>
#include <string>

struct Command;
class float3;
class CUnit;
class CTeam;
class CFeature;
struct UnitDef;
class SkirmishAIKey;

// This class is meant to contain COBject and its descendants only..
template<class A>
class CObject_pointer : public CObject
{
public:
	A* held;
	CObject_pointer() : held(NULL) {}
	template<class B> explicit CObject_pointer(B* u) : held(u) { AddDeathDependence((CObject *)held); }
	template<class B> explicit CObject_pointer(CObject_pointer<B> const& u) { AddDeathDependence((CObject*)held); }
	~CObject_pointer() { if (held) DeleteDeathDependence((CObject*)held); }
	virtual void DependentDied(CObject* o) { held = NULL; }
};

namespace luafunctions
{
	void CreateSkirmishAI(int teamId, const SkirmishAIKey& key,
			const std::map<std::string, std::string>& options = (std::map<std::string, std::string>()));
	void EndGame();
	void UnitGiveCommand(CObject_pointer<CUnit>* u, Command* c);
	CObject_pointer<CUnit>* UnitGetTransporter(CObject_pointer<CUnit>* u);
	//luabind::object GetUnitDefList( lua_State* L );
	int GetNumUnitDefs();
	//CObject_pointer<UnitDef>* GetUnitDefById( int id );
	CObject_pointer<CUnit>* UnitLoaderLoadUnit(std::string name, float3 pos, int team, bool buil);
	void RemoveUnit(CObject_pointer<CUnit>* u);
	CObject_pointer<CFeature>* FeatureLoaderLoadFeature( std::string name, float3 pos, int team );
	luabind::object GetFeaturesAt(lua_State* L, const float3& pos, float radius);
	int GetNumUnitsAt(const float3& pos, float radius);
	luabind::object GetUnitsAt(lua_State* L, const float3& pos, float radius);
	std::string MapGetTDFName();
	luabind::object GetSelectedUnits(lua_State* L, int player);
	void SendSelectedUnits();
	CTeam* GetTeam(int num);
};

#endif

