/* Author: Tobi Vollebregt */

#include "UnitScriptNames.h"
#include "CobFile.h" // COBFN_* constants


using std::map;
using std::pair;
using std::string;
using std::vector;


const vector<string>& CUnitScriptNames::GetScriptNames()
{
	static vector<string> scriptNames(COBFN_Last + (COB_MaxWeapons * COBFN_Weapon_Funcs));

	if (!scriptNames.empty()) {
		return scriptNames;
	}

	scriptNames[COBFN_Create]        = "Create";
	scriptNames[COBFN_StartMoving]   = "StartMoving";
	scriptNames[COBFN_StopMoving]    = "StopMoving";
	scriptNames[COBFN_Activate]      = "Activate";
	scriptNames[COBFN_Killed]        = "Killed";
	scriptNames[COBFN_Deactivate]    = "Deactivate";
	scriptNames[COBFN_SetDirection]  = "SetDirection";
	scriptNames[COBFN_SetSpeed]      = "SetSpeed";
	scriptNames[COBFN_RockUnit]      = "RockUnit";
	scriptNames[COBFN_HitByWeapon]   = "HitByWeapon";
	scriptNames[COBFN_MoveRate0]     = "MoveRate0";
	scriptNames[COBFN_MoveRate1]     = "MoveRate1";
	scriptNames[COBFN_MoveRate2]     = "MoveRate2";
	scriptNames[COBFN_MoveRate3]     = "MoveRate3";
	scriptNames[COBFN_SetSFXOccupy]  = "setSFXoccupy";
	scriptNames[COBFN_HitByWeaponId] = "HitByWeaponId";

	// Also add the weapon aiming stuff
	for (int i = 0; i < COB_MaxWeapons; ++i) {
		char buf[15];
		sprintf(buf, "Weapon%d", i + 1);
		string weapon(buf);
		sprintf(buf, "%d", i + 1);
		string weapnum(buf);
		scriptNames[COBFN_QueryPrimary   + i] = "Query"   + weapon;
		scriptNames[COBFN_AimPrimary     + i] = "Aim"     + weapon;
		scriptNames[COBFN_AimFromPrimary + i] = "AimFrom" + weapon;
		scriptNames[COBFN_FirePrimary    + i] = "Fire"    + weapon;
		scriptNames[COBFN_EndBurst       + i] = "EndBurst"     + weapnum;
		scriptNames[COBFN_Shot           + i] = "Shot"         + weapnum;
		scriptNames[COBFN_BlockShot      + i] = "BlockShot"    + weapnum;
		scriptNames[COBFN_TargetWeight   + i] = "TargetWeight" + weapnum;
	}

	return scriptNames;
}


const std::map<std::string, int>& CUnitScriptNames::GetScriptMap()
{
	static map<string, int> scriptMap;

	if (!scriptMap.empty()) {
		return scriptMap;
	}

	const vector<string>& n = GetScriptNames();
	for (size_t i = 0; i < n.size(); ++i) {
		scriptMap.insert(pair<string, int>(n[i], i));
	}
	// support the old naming scheme
	scriptMap.insert(pair<string, int>("QueryPrimary",     COBFN_QueryPrimary));
	scriptMap.insert(pair<string, int>("QuerySecondary",   COBFN_QueryPrimary + 1));
	scriptMap.insert(pair<string, int>("QueryTertiary",    COBFN_QueryPrimary + 2));
	scriptMap.insert(pair<string, int>("AimPrimary",       COBFN_AimPrimary));
	scriptMap.insert(pair<string, int>("AimSecondary",     COBFN_AimPrimary + 1));
	scriptMap.insert(pair<string, int>("AimTertiary",      COBFN_AimPrimary + 2));
	scriptMap.insert(pair<string, int>("AimFromPrimary",   COBFN_AimFromPrimary));
	scriptMap.insert(pair<string, int>("AimFromSecondary", COBFN_AimFromPrimary + 1));
	scriptMap.insert(pair<string, int>("AimFromTertiary",  COBFN_AimFromPrimary + 2));
	scriptMap.insert(pair<string, int>("FirePrimary",      COBFN_FirePrimary));
	scriptMap.insert(pair<string, int>("FireSecondary",    COBFN_FirePrimary + 1));
	scriptMap.insert(pair<string, int>("FireTertiary",     COBFN_FirePrimary + 2));

	return scriptMap;
}


int CUnitScriptNames::GetScriptNumber(const std::string& fname)
{
	const map<string, int>& scriptMap = GetScriptMap();
	map<string, int>::const_iterator it = scriptMap.find(fname);

	if (it != scriptMap.end()) {
		return it->second;
	}
	return -1;
}



const string& CUnitScriptNames::GetScriptName(int num)
{
	static string empty;
	const std::vector<std::string>& n = GetScriptNames();
	if (num >= 0 && num < int(n.size()))
		return n[num];
	return empty;
}
