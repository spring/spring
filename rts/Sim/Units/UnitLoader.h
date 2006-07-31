#ifndef UNITLOADER_H
#define UNITLOADER_H
// UnitLoader.h: interface for the CUnitLoader class.
//
//////////////////////////////////////////////////////////////////////

class CUnit;
class CWeapon;
#include <string>

#include "UnitDef.h"

using namespace std;
struct GuiSound;

class CUnitLoader  
{
public:
	CUnit* LoadUnit(const string& name,float3 pos, int side, bool build=true, int facing=0);
	CUnitLoader();
	virtual ~CUnitLoader();

protected:
	CWeapon* LoadWeapon(WeaponDef *weapondef, CUnit* owner,UnitDef::UnitDefWeapon* udw);
	void LoadSound(GuiSound &sound);
};

extern CUnitLoader unitLoader;

#endif /* UNITLOADER_H */
