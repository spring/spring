/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_RULESPARAMS_H
#define LUA_RULESPARAMS_H

#include <string>

#include "System/UnorderedMap.hpp"
#include "System/creg/creg_cond.h"

namespace LuaRulesParams
{
	enum {
		RULESPARAMLOS_PRIVATE = 1,  //! only readable by the ally (default)
		RULESPARAMLOS_ALLIED  = 2,  //! readable for ally + ingame allied
		RULESPARAMLOS_INLOS   = 4,  //! readable if the unit is in LOS
		RULESPARAMLOS_TYPED   = 8,  //! readable if the unit is typed (= in radar and was once in LOS)
		RULESPARAMLOS_INRADAR = 16, //! readable if the unit is in radar
		RULESPARAMLOS_PUBLIC  = 32, //! readable for all

		//! note: that the following constants include all states beneath it (e.g. PRIVATE_MASK includes PUBLIC,ALLIED,INLOS,...)
		RULESPARAMLOS_PRIVATE_MASK = 63,
		RULESPARAMLOS_ALLIED_MASK  = 62,
		RULESPARAMLOS_INLOS_MASK   = 60,
		RULESPARAMLOS_TYPED_MASK   = 56,
		RULESPARAMLOS_INRADAR_MASK = 48,
		RULESPARAMLOS_PUBLIC_MASK  = 32
	};

	struct Param {
		CR_DECLARE_STRUCT(Param)

		Param() : los(RULESPARAMLOS_PRIVATE),valueInt(0.0f) {};

		int   los;
		float valueInt;
		std::string valueString;
	};

	typedef spring::unordered_map<std::string, Param> Params;
}

#endif // LUA_RULESPARAMS_H
