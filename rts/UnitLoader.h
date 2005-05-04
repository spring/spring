// UnitLoader.h: interface for the CUnitLoader class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UNITLOADER_H__91B355AB_7C1B_4DFB_AB74_F6AED112DED7__INCLUDED_)
#define AFX_UNITLOADER_H__91B355AB_7C1B_4DFB_AB74_F6AED112DED7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CUnitParser;
class CUnit;
class CWeapon;
#include <string>

#include "unitdef.h"

using namespace std;
struct GuiSound;

class CUnitLoader  
{
public:
	CUnit* LoadUnit(const string& name,float3 pos, int side, bool build=true);
	CUnitLoader();
	virtual ~CUnitLoader();

protected:
	CWeapon* LoadWeapon(WeaponDef *weapondef, CUnit* owner);
	void LoadSound(GuiSound &sound);
public:
	bool CanBuildUnit(string name, int team);
	int GetTechLevel(string type);
	string GetName(string type);
};

extern CUnitLoader unitLoader;

#endif // !defined(AFX_UNITLOADER_H__91B355AB_7C1B_4DFB_AB74_F6AED112DED7__INCLUDED_)
