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
#include "System/Util.h"


CR_BIND(CDamageArrayHandler, (NULL))

CR_REG_METADATA(CDamageArrayHandler, (
	CR_MEMBER(armorDefNameIdxMap),
	CR_MEMBER(armorDefKeys),
	CR_RESERVED(16)
))


CDamageArrayHandler* damageArrayHandler;


CDamageArrayHandler::CDamageArrayHandler(LuaParser* defsParser)
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

			if (SpringVersion::GetMajor()[1] >= '4') {
				std::vector<std::string> armorDefTableKeys;
				armorDefTable.GetKeys(armorDefTableKeys);

				// do not continue, table might ALSO have array-style entries
				if (!armorDefTableKeys.empty()) {
					LOG_L(L_WARNING,
						"[%s] ArmorDefs contains sub-table \"%s\" in <key, value> "
						"format which is deprecated as of Spring 95.0 and will not "
						"be parsed anymore (UPDATE YOUR armordefs.lua ASAP)!\n",
						__FUNCTION__, armorDefName.c_str());
				}
			}

			for (unsigned int armorDefEntryIdx = 0; armorDefEntryIdx < numArmorDefEntries; armorDefEntryIdx++) {
				const std::string unitDefName = StringToLower(armorDefTable.GetString(armorDefEntryIdx + 1, ""));
				const std::map<std::string, int>::const_iterator armorDefTableIt = armorDefNameIdxMap.find(unitDefName);

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
	const std::map<std::string, int>::const_iterator it = armorDefNameIdxMap.find(StringToLower(name));

	if (it != armorDefNameIdxMap.end()) {
		return it->second;
	}

	return 0; // 'default' armor index
}

