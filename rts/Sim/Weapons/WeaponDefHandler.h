/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPONDEFHANDLER_H
#define WEAPONDEFHANDLER_H

#include <string>
#include <vector>

#include "Sim/Misc/CommonDefHandler.h"
#include "Sim/Misc/GuiSoundSet.h"
#include "WeaponDef.h"
#include "System/float3.h"

class LuaParser;
class LuaTable;

class CWeaponDefHandler : CommonDefHandler
{
public:
	void Init(LuaParser* defsParser);
	void Kill() {
		weaponDefsVector.clear();
		weaponDefIDs.clear(); // never iterated
	}

	bool IsValidWeaponDefID(const int id) const {
		return (id >= 0) && (static_cast<size_t>(id) < weaponDefsVector.size());
	}

	// id=0 *is* a valid WeaponDef, hence no -1
	unsigned int NumWeaponDefs() const { return (weaponDefsVector.size()); }

	// NOTE: safe with unordered_map after Init
	const WeaponDef* GetWeaponDef(std::string wdName) const;
	const WeaponDef* GetWeaponDefByID(int id) const;

	const std::vector<WeaponDef>& GetWeaponDefsVec() const { return weaponDefsVector; }

private:
	std::vector<WeaponDef> weaponDefsVector;
	spring::unordered_map<std::string, int> weaponDefIDs;
};



extern CWeaponDefHandler* weaponDefHandler;


#endif /* WEAPONDEFHANDLER_H */
