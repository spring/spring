/* Author: Tobi Vollebregt */

#ifndef UNITSCRIPTNAMES_H
#define UNITSCRIPTNAMES_H


#include <map>
#include <string>
#include <vector>


//These are mapped by the CCobFile at startup to make common function calls faster
const int COBFN_Create = 0;
const int COBFN_StartMoving = 1;
const int COBFN_StopMoving = 2;
const int COBFN_Activate = 3;
const int COBFN_Killed = 4;
const int COBFN_Deactivate = 5;
const int COBFN_SetDirection = 6;
const int COBFN_SetSpeed = 7;
const int COBFN_RockUnit = 8;
const int COBFN_HitByWeapon = 9;
const int COBFN_MoveRate0 = 10;
const int COBFN_MoveRate1 = 11;
const int COBFN_MoveRate2 = 12;
const int COBFN_MoveRate3 = 13;
const int COBFN_SetSFXOccupy = 14;
const int COBFN_HitByWeaponId = 15;
const int COBFN_Last = 16;					//Make sure to update this, so the array will be sized properly

// These are special (they need space for MaxWeapons of each)
const int COB_MaxWeapons = 32;
const int COBFN_QueryPrimary   = COBFN_Last;
const int COBFN_AimPrimary     = COBFN_QueryPrimary   + COB_MaxWeapons;
const int COBFN_AimFromPrimary = COBFN_AimPrimary     + COB_MaxWeapons;
const int COBFN_FirePrimary    = COBFN_AimFromPrimary + COB_MaxWeapons;
const int COBFN_EndBurst       = COBFN_FirePrimary    + COB_MaxWeapons;
const int COBFN_Shot           = COBFN_EndBurst       + COB_MaxWeapons;
const int COBFN_BlockShot      = COBFN_Shot           + COB_MaxWeapons;
const int COBFN_TargetWeight   = COBFN_BlockShot      + COB_MaxWeapons;
const int COBFN_Weapon_Funcs   = 8;


class CUnitScriptNames
{
public:
	static const std::vector<std::string>& GetScriptNames(); // COBFN_* -> string
	static const std::map<std::string, int>& GetScriptMap(); // string -> COBFN_*

	static int GetScriptNumber(const std::string& fname);
	static const std::string& GetScriptName(int num);
};

#endif
