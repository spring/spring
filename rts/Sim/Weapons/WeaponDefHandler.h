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

	void LoadSound(const LuaTable&, GuiSoundSet&, const std::string& name);

	DamageArray DynamicDamages(DamageArray damages, float3 startPos,
					float3 curPos, float range, float exp,
					float damageMin, bool inverted);

public:
	WeaponDef *weaponDefs;
	std::map<std::string, int> weaponID;
	int numWeaponDefs;

private:
	void ParseWeapon(const LuaTable& wdTable, WeaponDef& wd);
	float3 hs2rgb(float h, float s);
};



extern CWeaponDefHandler* weaponDefHandler;


#endif /* WEAPONDEFHANDLER_H */
