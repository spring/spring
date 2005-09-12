#ifndef UNITLOADER_H
#define UNITLOADER_H
// UnitLoader.h: interface for the CUnitLoader class.
//
//////////////////////////////////////////////////////////////////////

class CUnitParser;
class CUnit;
class CWeapon;
#include <string>

#include "UnitDef.h"

using namespace std;
struct GuiSound;

class CUnitLoader  
{
public:
	CUnit* LoadUnit(const string& name,float3 pos, int side, bool build=true);
	CUnitLoader();
	virtual ~CUnitLoader();

protected:
	CWeapon* LoadWeapon(WeaponDef *weapondef, CUnit* owner,UnitDef::UnitDefWeapon* udw);
	void LoadSound(GuiSound &sound);
public:
	bool CanBuildUnit(string name, int team);
	int GetTechLevel(string type);
	string GetName(string type);
};

extern CUnitLoader unitLoader;

#endif /* UNITLOADER_H */
