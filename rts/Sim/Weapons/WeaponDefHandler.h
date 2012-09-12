/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPONDEFHANDLER_H
#define WEAPONDEFHANDLER_H

#include <string>
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

	const WeaponDef* GetWeapon(const std::string& weaponname);
	const WeaponDef* GetWeaponById(int weaponDefId);

	void LoadSound(
		const LuaTable&,
		const std::string& soundKey,
		const unsigned int soundIdx,
		std::vector<GuiSoundSet::Data>&
	);

	static DamageArray DynamicDamages(const DamageArray damages, const float3 startPos,
					const float3 curPos, const float range, const float exp,
					const float damageMin, const bool inverted);

public:
	std::vector<WeaponDef> weaponDefs;
	std::map<std::string, int> weaponID;

private:
	void ParseWeapon(const LuaTable& wdTable, WeaponDef& wd);
	void ParseWeaponVisuals(const LuaTable& wdTable, WeaponDef& wd);
	void ParseWeaponSounds(const LuaTable& wdTable, WeaponDef& wd);
};



extern CWeaponDefHandler* weaponDefHandler;


#endif /* WEAPONDEFHANDLER_H */
