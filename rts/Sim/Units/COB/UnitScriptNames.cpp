/* Author: Tobi Vollebregt */

#include "UnitScriptNames.h"
#include "LogOutput.h"


using std::map;
using std::pair;
using std::string;
using std::vector;


const vector<string>& CUnitScriptNames::GetScriptNames()
{
	static vector<string> scriptNames;

	if (!scriptNames.empty()) {
		return scriptNames;
	}

	scriptNames.resize(COBFN_Last + (COB_MaxWeapons * COBFN_Weapon_Funcs));

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

	scriptNames[COBFN_QueryLandingPadCount] = "QueryLandingPadCount";
	scriptNames[COBFN_QueryLandingPad]      = "QueryLandingPad";
	scriptNames[COBFN_Falling]              = "Falling";
	scriptNames[COBFN_Landed]               = "Landed";
	scriptNames[COBFN_BeginTransport]       = "BeginTransport";
	scriptNames[COBFN_QueryTransport]       = "QueryTransport";
	scriptNames[COBFN_TransportPickup]      = "TransportPickup";
	scriptNames[COBFN_StartUnload]          = "StartUnload";
	scriptNames[COBFN_EndTransport]         = "EndTransport";
	scriptNames[COBFN_TransportDrop]        = "TransportDrop";
	scriptNames[COBFN_SetMaxReloadTime]     = "SetMaxReloadTime";
	scriptNames[COBFN_StartBuilding]        = "StartBuilding";
	scriptNames[COBFN_StopBuilding]         = "StopBuilding";
	scriptNames[COBFN_QueryNanoPiece]       = "QueryNanoPiece";
	scriptNames[COBFN_QueryBuildInfo]       = "QueryBuildInfo";
	scriptNames[COBFN_Go]                   = "Go";

	scriptNames[COBFN_MoveFinished] = "MoveFinished";
	scriptNames[COBFN_TurnFinished] = "TurnFinished";

	// Also add the weapon aiming stuff
	for (int i = 0; i < COB_MaxWeapons; ++i) {
		char buf[15];
		sprintf(buf, "Weapon%d", i + 1);
		string weapon(buf);
		sprintf(buf, "%d", i + 1);
		string weapnum(buf);
		const int n = COBFN_Weapon_Funcs * i;
		scriptNames[COBFN_QueryPrimary   + n] = "Query"   + weapon;
		scriptNames[COBFN_AimPrimary     + n] = "Aim"     + weapon;
		scriptNames[COBFN_AimFromPrimary + n] = "AimFrom" + weapon;
		scriptNames[COBFN_FirePrimary    + n] = "Fire"    + weapon;
		scriptNames[COBFN_EndBurst       + n] = "EndBurst"     + weapnum;
		scriptNames[COBFN_Shot           + n] = "Shot"         + weapnum;
		scriptNames[COBFN_BlockShot      + n] = "BlockShot"    + weapnum;
		scriptNames[COBFN_TargetWeight   + n] = "TargetWeight" + weapnum;
	}

	//for (size_t i = 0; i < scriptNames.size(); ++i) {
	//	logOutput.Print("COBFN: %3d %s", i, scriptNames[i].c_str());
	//}

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
	scriptMap.insert(pair<string, int>("QuerySecondary",   COBFN_QueryPrimary + COBFN_Weapon_Funcs * 1));
	scriptMap.insert(pair<string, int>("QueryTertiary",    COBFN_QueryPrimary + COBFN_Weapon_Funcs * 2));
	scriptMap.insert(pair<string, int>("AimPrimary",       COBFN_AimPrimary));
	scriptMap.insert(pair<string, int>("AimSecondary",     COBFN_AimPrimary + COBFN_Weapon_Funcs * 1));
	scriptMap.insert(pair<string, int>("AimTertiary",      COBFN_AimPrimary + COBFN_Weapon_Funcs * 2));
	scriptMap.insert(pair<string, int>("AimFromPrimary",   COBFN_AimFromPrimary));
	scriptMap.insert(pair<string, int>("AimFromSecondary", COBFN_AimFromPrimary + COBFN_Weapon_Funcs * 1));
	scriptMap.insert(pair<string, int>("AimFromTertiary",  COBFN_AimFromPrimary + COBFN_Weapon_Funcs * 2));
	scriptMap.insert(pair<string, int>("FirePrimary",      COBFN_FirePrimary));
	scriptMap.insert(pair<string, int>("FireSecondary",    COBFN_FirePrimary + COBFN_Weapon_Funcs * 1));
	scriptMap.insert(pair<string, int>("FireTertiary",     COBFN_FirePrimary + COBFN_Weapon_Funcs * 2));

	//for (std::map<string, int>::const_iterator it = scriptMap.begin(); it != scriptMap.end(); ++it) {
	//	logOutput.Print("COBFN: %s -> %3d", it->first.c_str(), it->second);
	//}

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
