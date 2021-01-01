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

		// note: the following constants include all states beneath them (e.g. PRIVATE_MASK includes PUBLIC,ALLIED,INLOS,...)
		RULESPARAMLOS_PRIVATE_MASK = RULESPARAMLOS_PUBLIC | RULESPARAMLOS_INRADAR | RULESPARAMLOS_TYPED | RULESPARAMLOS_INLOS | RULESPARAMLOS_ALLIED | RULESPARAMLOS_PRIVATE,
		RULESPARAMLOS_ALLIED_MASK  = RULESPARAMLOS_PUBLIC | RULESPARAMLOS_INRADAR | RULESPARAMLOS_TYPED | RULESPARAMLOS_INLOS | RULESPARAMLOS_ALLIED,
		RULESPARAMLOS_INLOS_MASK   = RULESPARAMLOS_PUBLIC | RULESPARAMLOS_INRADAR | RULESPARAMLOS_TYPED | RULESPARAMLOS_INLOS,
		RULESPARAMLOS_TYPED_MASK   = RULESPARAMLOS_PUBLIC | RULESPARAMLOS_INRADAR | RULESPARAMLOS_TYPED,
		RULESPARAMLOS_INRADAR_MASK = RULESPARAMLOS_PUBLIC | RULESPARAMLOS_INRADAR,
		RULESPARAMLOS_PUBLIC_MASK  = RULESPARAMLOS_PUBLIC
	};

	struct Param {
		CR_DECLARE_STRUCT(Param)

		int   los = RULESPARAMLOS_PRIVATE;
		float valueInt = 0.0f;
		std::string valueString;
	};

	typedef spring::unordered_map<std::string, Param> Params;
}

#endif // LUA_RULESPARAMS_H
