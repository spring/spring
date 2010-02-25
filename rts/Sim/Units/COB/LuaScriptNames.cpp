/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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

	scriptNames.resize(LUAFN_Last);

	scriptNames[LUAFN_Destroy]       = "Destroy";
	scriptNames[LUAFN_StartMoving]   = "StartMoving";
	scriptNames[LUAFN_StopMoving]    = "StopMoving";
	scriptNames[LUAFN_Activate]      = "Activate";
	scriptNames[LUAFN_Killed]        = "Killed";
	scriptNames[LUAFN_Deactivate]    = "Deactivate";
	scriptNames[LUAFN_WindChanged]   = "WindChanged";
	scriptNames[LUAFN_ExtractionRateChanged] = "ExtractionRateChanged";
	scriptNames[LUAFN_RockUnit]      = "RockUnit";
	scriptNames[LUAFN_MoveRate]      = "MoveRate";
	scriptNames[LUAFN_SetSFXOccupy]  = "setSFXoccupy";
	scriptNames[LUAFN_HitByWeapon]   = "HitByWeapon";

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

	scriptNames[LUAFN_MoveFinished] = "MoveFinished";
	scriptNames[LUAFN_TurnFinished] = "TurnFinished";

	// Also add the weapon aiming stuff
	scriptNames[LUAFN_QueryWeapon]   = "QueryWeapon";
	scriptNames[LUAFN_AimWeapon]     = "AimWeapon";
	scriptNames[LUAFN_AimShield]     = "AimShield"; // TODO: maybe not the best name?
	scriptNames[LUAFN_AimFromWeapon] = "AimFromWeapon";
	scriptNames[LUAFN_FireWeapon]    = "FireWeapon";
	scriptNames[LUAFN_EndBurst]      = "EndBurst";
	scriptNames[LUAFN_Shot]          = "Shot";
	scriptNames[LUAFN_BlockShot]     = "BlockShot";
	scriptNames[LUAFN_TargetWeight]  = "TargetWeight";

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
