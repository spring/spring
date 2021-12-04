/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DAMAGE_ARRAY_HANDLER_H
#define _DAMAGE_ARRAY_HANDLER_H

#include <string>
#include <vector>

#include "System/Misc/NonCopyable.h"
#include "System/creg/creg_cond.h"
#include "System/UnorderedMap.hpp"

class LuaParser;
class CDamageArrayHandler : public spring::noncopyable
{
	CR_DECLARE_STRUCT(CDamageArrayHandler)

public:
	void Init(LuaParser* defsParser);
	void Kill() {
		armorDefNameIdxMap.clear(); // never iterated
		armorDefKeys.clear();
	}

	int GetTypeFromName(const std::string& name) const;
	int GetNumTypes() const { return armorDefKeys.size(); }

	const std::vector<std::string>& GetTypeList() const { return armorDefKeys; }

private:
	spring::unordered_map<std::string, int> armorDefNameIdxMap;
	std::vector<std::string> armorDefKeys;
};

extern CDamageArrayHandler damageArrayHandler;

#endif // _DAMAGE_ARRAY_HANDLER_H
