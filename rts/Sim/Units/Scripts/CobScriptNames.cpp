/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CobScriptNames.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/StringUtil.h"

// script function-indices never change, so this is fine wrt. reloading
static std::array<std::string, COBFN_NumUnitFuncs> scriptNames;
static spring::unordered_map<std::string, int> scriptMap;


const std::array<std::string, COBFN_NumUnitFuncs>& CCobUnitScriptNames::GetScriptNames()
{
	if (!scriptNames[COBFN_Create].empty())
		return scriptNames;

	scriptNames[COBFN_Create]        = "Create";
	scriptNames[COBFN_Destroy]       = "Destroy";
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

	// Also add the weapon aiming stuff
	for (int i = 0; i < MAX_WEAPONS_PER_UNIT; ++i) {
		const std::string wn = IntToString(i + 1, "%d");
		const int n = COBFN_Weapon_Funcs * i;

		scriptNames[COBFN_QueryPrimary   + n] = "QueryWeapon"   + wn;
		scriptNames[COBFN_AimPrimary     + n] = "AimWeapon"     + wn;
		scriptNames[COBFN_AimFromPrimary + n] = "AimFromWeapon" + wn;
		scriptNames[COBFN_FirePrimary    + n] = "FireWeapon"    + wn;
		scriptNames[COBFN_EndBurst       + n] = "EndBurst"      + wn;
		scriptNames[COBFN_Shot           + n] = "Shot"          + wn;
		scriptNames[COBFN_BlockShot      + n] = "BlockShot"     + wn;
		scriptNames[COBFN_TargetWeight   + n] = "TargetWeight"  + wn;
	}

	return scriptNames;
}


const spring::unordered_map<std::string, int>& CCobUnitScriptNames::GetScriptMap()
{
	if (!scriptMap.empty())
		return scriptMap;

	const auto& n = GetScriptNames();

	scriptMap.reserve(n.size() + 4 * 3);

	for (size_t i = 0; i < n.size(); ++i) {
		scriptMap.insert(n[i], i);
	}

	// support the old naming scheme
	scriptMap.insert("QueryPrimary",     COBFN_QueryPrimary                           );
	scriptMap.insert("QuerySecondary",   COBFN_QueryPrimary   + COBFN_Weapon_Funcs * 1);
	scriptMap.insert("QueryTertiary",    COBFN_QueryPrimary   + COBFN_Weapon_Funcs * 2);
	scriptMap.insert("AimPrimary",       COBFN_AimPrimary                             );
	scriptMap.insert("AimSecondary",     COBFN_AimPrimary     + COBFN_Weapon_Funcs * 1);
	scriptMap.insert("AimTertiary",      COBFN_AimPrimary     + COBFN_Weapon_Funcs * 2);
	scriptMap.insert("AimFromPrimary",   COBFN_AimFromPrimary                         );
	scriptMap.insert("AimFromSecondary", COBFN_AimFromPrimary + COBFN_Weapon_Funcs * 1);
	scriptMap.insert("AimFromTertiary",  COBFN_AimFromPrimary + COBFN_Weapon_Funcs * 2);
	scriptMap.insert("FirePrimary",      COBFN_FirePrimary                            );
	scriptMap.insert("FireSecondary",    COBFN_FirePrimary    + COBFN_Weapon_Funcs * 1);
	scriptMap.insert("FireTertiary",     COBFN_FirePrimary    + COBFN_Weapon_Funcs * 2);

	return scriptMap;
}


int CCobUnitScriptNames::GetScriptNumber(const std::string& fname)
{
	const auto& scriptMap = GetScriptMap();
	const auto it = scriptMap.find(fname);

	if (it != scriptMap.end())
		return it->second;

	return -1;
}

const std::string& CCobUnitScriptNames::GetScriptName(int num)
{
	const static std::string empty;
	const auto& n = GetScriptNames();

	if (num >= 0 && num < int(n.size()))
		return n[num];

	return empty;
}

