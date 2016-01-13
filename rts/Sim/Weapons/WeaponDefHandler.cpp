/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include "WeaponDefHandler.h"

#include "Lua/LuaParser.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "System/Exceptions.h"
#include "System/Util.h"
#include "System/Log/ILog.h"


CWeaponDefHandler* weaponDefHandler = NULL;


CWeaponDefHandler::CWeaponDefHandler(LuaParser* defsParser)
{
	const LuaTable rootTable = defsParser->GetRoot().SubTable("WeaponDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading WeaponDefs");
	}

	vector<string> weaponNames;
	rootTable.GetKeys(weaponNames);

	weaponDefs.resize(weaponNames.size());

	for (int wid = 0; wid < weaponDefs.size(); wid++) {
		const std::string& name = weaponNames[wid];
		const LuaTable wdTable = rootTable.SubTable(name);
		weaponDefs[wid] = WeaponDef(wdTable, name, wid);
		weaponID[name] = wid;
	}
}


CWeaponDefHandler::~CWeaponDefHandler()
{
}



const WeaponDef* CWeaponDefHandler::GetWeaponDef(std::string weaponname) const
{
	StringToLowerInPlace(weaponname);

	std::map<std::string,int>::const_iterator ii = weaponID.find(weaponname);
	if (ii == weaponID.end())
		return NULL;

	return &weaponDefs[ii->second];
}


const WeaponDef* CWeaponDefHandler::GetWeaponDefByID(int weaponDefId) const
{
	if ((weaponDefId < 0) || (weaponDefId >= weaponDefs.size())) {
		return NULL;
	}
	return &weaponDefs[weaponDefId];
}



DamageArray CWeaponDefHandler::DynamicDamages(
	const WeaponDef* weaponDef,
	const float3 startPos,
	const float3 curPos
) {
	const DamageArray& damages = weaponDef->damages;

	if (weaponDef->damages.dynDamageExp <= 0.0f)
		return damages;

	DamageArray dynDamages(damages);

	const float range     = (weaponDef->damages.dynDamageRange > 0.0f)? weaponDef->damages.dynDamageRange: weaponDef->range;
	const float damageMin = weaponDef->damages.dynDamageMin;

	const float travDist  = std::min(range, curPos.distance2D(startPos));
	const float damageMod = 1.0f - math::pow(1.0f / range * travDist, weaponDef->damages.dynDamageExp);
	const float ddmod     = damageMin / damages[0]; // get damage mod from first damage type

	if (weaponDef->damages.dynDamageInverted) {
		for (int i = 0; i < damageArrayHandler->GetNumTypes(); ++i) {
			dynDamages[i] = damages[i] - damageMod * damages[i];

			if (damageMin > 0.0f)
				dynDamages[i] = std::max(damages[i] * ddmod, dynDamages[i]);

			// to prevent div by 0
			dynDamages[i] = std::max(0.0001f, dynDamages[i]);
		}
	} else {
		for (int i = 0; i < damageArrayHandler->GetNumTypes(); ++i) {
			dynDamages[i] = damageMod * damages[i];

			if (damageMin > 0.0f)
				dynDamages[i] = std::max(damages[i] * ddmod, dynDamages[i]);

			// div by 0
			dynDamages[i] = std::max(0.0001f, dynDamages[i]);
		}
	}

	return dynDamages;
}
