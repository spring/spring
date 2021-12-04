/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <locale>
#include <string>
#include <vector>
#include <cctype>

#include "DamageArrayHandler.h"
#include "Game/GameVersion.h"
#include "Lua/LuaParser.h"
#include "System/creg/STL_Map.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/StringUtil.h"


CR_BIND(CDamageArrayHandler, )

CR_REG_METADATA(CDamageArrayHandler, (
	CR_MEMBER(armorDefNameIdxMap),
	CR_MEMBER(armorDefKeys)
))


CDamageArrayHandler damageArrayHandler;


void CDamageArrayHandler::Init(LuaParser* defsParser)
{
	#define DEFAULT_ARMORDEF_NAME "default"

	try {
		const LuaTable rootTable = defsParser->GetRoot().SubTable("ArmorDefs");

		if (!rootTable.IsValid())
			throw content_error("Error loading ArmorDefs");

		// GetKeys() sorts the keys, so can not simply push_back before call
		rootTable.GetKeys(armorDefKeys);
		armorDefKeys.insert(armorDefKeys.begin(), DEFAULT_ARMORDEF_NAME);

		armorDefNameIdxMap[DEFAULT_ARMORDEF_NAME] = 0;

		LOG("[%s] number of ArmorDefs: " _STPF_, __FUNCTION__, armorDefKeys.size());

		// expects the following structure, subtables must be in array-format:
		//
		// {"tanks" = {[1] = "supertank", [2] = "megatank"}, "infantry" = {[1] = "dude"}, ...}
		//
		// the old (pre-95.0) <key, value> subtable definitions are no longer supported!
		//
		for (unsigned int armorDefIdx = 1; armorDefIdx < armorDefKeys.size(); armorDefIdx++) {
			const std::string armorDefName = StringToLower(armorDefKeys[armorDefIdx]);

			if (armorDefName == DEFAULT_ARMORDEF_NAME) {
				// ignore, no need to clear entire table
				LOG_L(L_WARNING, "[%s] ArmorDefs: tried to define the \"%s\" armor type!", __FUNCTION__, DEFAULT_ARMORDEF_NAME);
				continue;
			}

			armorDefNameIdxMap[armorDefName] = armorDefIdx;

			const LuaTable armorDefTable = rootTable.SubTable(armorDefKeys[armorDefIdx]);
			const unsigned int numArmorDefEntries = armorDefTable.GetLength();

			for (unsigned int armorDefEntryIdx = 0; armorDefEntryIdx < numArmorDefEntries; armorDefEntryIdx++) {
				const std::string& unitDefName = StringToLower(armorDefTable.GetString(armorDefEntryIdx + 1, ""));
				const auto armorDefTableIt = armorDefNameIdxMap.find(unitDefName);

				if (armorDefTableIt == armorDefNameIdxMap.end()) {
					armorDefNameIdxMap[unitDefName] = armorDefIdx;
					continue;
				}

				LOG_L(L_WARNING,
					"[%s] UnitDef \"%s\" in ArmorDef \"%s\" already belongs to ArmorDef category %d!",
					__FUNCTION__, unitDefName.c_str(), armorDefName.c_str(), armorDefTableIt->second);
			}
		}
	} catch (const content_error&) {
		armorDefNameIdxMap.clear();
		armorDefNameIdxMap[DEFAULT_ARMORDEF_NAME] = 0;

		armorDefKeys.clear();
		armorDefKeys.push_back(DEFAULT_ARMORDEF_NAME);
	}
}



int CDamageArrayHandler::GetTypeFromName(const std::string& name) const
{
	const auto it = armorDefNameIdxMap.find(StringToLower(name));

	if (it != armorDefNameIdxMap.end())
		return it->second;

	return 0; // 'default' armor index
}

