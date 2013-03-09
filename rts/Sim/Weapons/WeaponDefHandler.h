/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPONDEFHANDLER_H
#define WEAPONDEFHANDLER_H

#include <string>
#include <vector>
#include <map>

#include "Sim/Misc/CommonDefHandler.h"
#include "Sim/Misc/GuiSoundSet.h"
#include "WeaponDef.h"
#include "System/float3.h"

class LuaTable;

class CWeaponDefHandler : CommonDefHandler
{
public:
	CWeaponDefHandler();
	~CWeaponDefHandler();

	const WeaponDef* GetWeaponDef(std::string weaponname) const;
	const WeaponDef* GetWeaponDefByID(int weaponDefId) const;

	static DamageArray DynamicDamages(const DamageArray& damages, const float3 startPos,
					const float3 curPos, const float range, const float exp,
					const float damageMin, const bool inverted);

public:
	std::vector<WeaponDef> weaponDefs;
	std::map<std::string, int> weaponID;
};



extern CWeaponDefHandler* weaponDefHandler;


#endif /* WEAPONDEFHANDLER_H */
