#ifndef __LUA_FUNCTIONS
#define __LUA_FUNCTIONS

#include "LuaBinder.h"
#include "Object.h"

struct Command;
class float3;
class CUnit;

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
	void EndGame();
	void UnitGiveCommand(CObject_pointer<CUnit>* u, Command* c);
	CObject_pointer<CUnit>* UnitGetTransporter(CObject_pointer<CUnit>* u);
	void CommandAddParam(Command* c, float p);
	CObject_pointer<CUnit>* UnitLoaderLoadUnit(std::string name, float3 pos, int team, bool buil);
	int GetNumUnitsAt(const float3& pos, float radius);
};

#endif

