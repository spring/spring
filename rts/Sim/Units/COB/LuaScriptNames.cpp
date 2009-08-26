/* Author: Tobi Vollebregt */

#include <cstdio>

#include "Sim/Misc/GlobalConstants.h"
#include "LuaScriptNames.h"
#include "LogOutput.h"

using std::sprintf;

using std::map;
using std::pair;
using std::string;
using std::vector;


const vector<string>& CLuaUnitScriptNames::GetScriptNames()
{
	static vector<string> scriptNames;

	if (!scriptNames.empty()) {
		return scriptNames;
	}

	scriptNames.resize(LUAFN_Last + (MAX_WEAPONS_PER_UNIT * LUAFN_Weapon_Funcs));

	scriptNames[LUAFN_Destroy]       = "Destroy";
	scriptNames[LUAFN_StartMoving]   = "StartMoving";
	scriptNames[LUAFN_StopMoving]    = "StopMoving";
	scriptNames[LUAFN_Activate]      = "Activate";
	scriptNames[LUAFN_Killed]        = "Killed";
	scriptNames[LUAFN_Deactivate]    = "Deactivate";
	scriptNames[LUAFN_SetDirection]  = "SetDirection";
	scriptNames[LUAFN_SetSpeed]      = "SetSpeed";
	scriptNames[LUAFN_RockUnit]      = "RockUnit";
	scriptNames[LUAFN_HitByWeapon]   = "HitByWeapon";
	scriptNames[LUAFN_MoveRate0]     = "MoveRate0";
	scriptNames[LUAFN_MoveRate1]     = "MoveRate1";
	scriptNames[LUAFN_MoveRate2]     = "MoveRate2";
	scriptNames[LUAFN_MoveRate3]     = "MoveRate3";
	scriptNames[LUAFN_SetSFXOccupy]  = "setSFXoccupy";
	scriptNames[LUAFN_HitByWeaponId] = "HitByWeaponId";

	scriptNames[LUAFN_QueryLandingPads]     = "QueryLandingPads";
	scriptNames[LUAFN_Falling]              = "Falling";
	scriptNames[LUAFN_Landed]               = "Landed";
	scriptNames[LUAFN_BeginTransport]       = "BeginTransport";
	scriptNames[LUAFN_QueryTransport]       = "QueryTransport";
	scriptNames[LUAFN_TransportPickup]      = "TransportPickup";
	scriptNames[LUAFN_StartUnload]          = "StartUnload";
	scriptNames[LUAFN_EndTransport]         = "EndTransport";
	scriptNames[LUAFN_TransportDrop]        = "TransportDrop";
	scriptNames[LUAFN_StartBuilding]        = "StartBuilding";
	scriptNames[LUAFN_StopBuilding]         = "StopBuilding";
	scriptNames[LUAFN_QueryNanoPiece]       = "QueryNanoPiece";
	scriptNames[LUAFN_QueryBuildInfo]       = "QueryBuildInfo";
	scriptNames[LUAFN_Go]                   = "Go";

	scriptNames[LUAFN_MoveFinished] = "MoveFinished";
	scriptNames[LUAFN_TurnFinished] = "TurnFinished";

	// Also add the weapon aiming stuff
	for (int i = 0; i < MAX_WEAPONS_PER_UNIT; ++i) {
		char buf[15];
		sprintf(buf, "Weapon%d", i + 1);
		string weapon(buf);
		sprintf(buf, "%d", i + 1);
		string weapnum(buf);
		const int n = LUAFN_Weapon_Funcs * i;
		scriptNames[LUAFN_QueryPrimary   + n] = "Query"   + weapon;
		scriptNames[LUAFN_AimPrimary     + n] = "Aim"     + weapon;
		scriptNames[LUAFN_AimFromPrimary + n] = "AimFrom" + weapon;
		scriptNames[LUAFN_FirePrimary    + n] = "Fire"    + weapon;
		scriptNames[LUAFN_EndBurst       + n] = "EndBurst"     + weapnum;
		scriptNames[LUAFN_Shot           + n] = "Shot"         + weapnum;
		scriptNames[LUAFN_BlockShot      + n] = "BlockShot"    + weapnum;
		scriptNames[LUAFN_TargetWeight   + n] = "TargetWeight" + weapnum;
	}

	//for (size_t i = 0; i < scriptNames.size(); ++i) {
	//	logOutput.Print("LUAFN: %3d %s", i, scriptNames[i].c_str());
	//}

	return scriptNames;
}


const std::map<std::string, int>& CLuaUnitScriptNames::GetScriptMap()
{
	static map<string, int> scriptMap;

	if (!scriptMap.empty()) {
		return scriptMap;
	}

	const vector<string>& n = GetScriptNames();
	for (size_t i = 0; i < n.size(); ++i) {
		scriptMap.insert(pair<string, int>(n[i], i));
	}

	//for (std::map<string, int>::const_iterator it = scriptMap.begin(); it != scriptMap.end(); ++it) {
	//	logOutput.Print("LUAFN: %s -> %3d", it->first.c_str(), it->second);
	//}

	return scriptMap;
}


int CLuaUnitScriptNames::GetScriptNumber(const std::string& fname)
{
	const map<string, int>& scriptMap = GetScriptMap();
	map<string, int>::const_iterator it = scriptMap.find(fname);

	if (it != scriptMap.end()) {
		return it->second;
	}
	return -1;
}


const string& CLuaUnitScriptNames::GetScriptName(int num)
{
	static string empty;
	const std::vector<std::string>& n = GetScriptNames();
	if (num >= 0 && num < int(n.size()))
		return n[num];
	return empty;
}
