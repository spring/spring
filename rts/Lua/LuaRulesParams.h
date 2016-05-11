/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_RULESPARAMS_H
#define LUA_RULESPARAMS_H

#include <string>
#include <unordered_map>
#include "System/creg/creg_cond.h"

namespace LuaRulesParams
{
	enum {
		RULESPARAMLOS_PRIVATE = 1,  //! only readable by the ally (default)
		RULESPARAMLOS_ALLIED  = 2,  //! readable for ally + ingame allied
		RULESPARAMLOS_INLOS   = 4,  //! readable if the unit is in LOS
		RULESPARAMLOS_INRADAR = 8,  //! readable if the unit is in AirLOS
		RULESPARAMLOS_PUBLIC  = 16, //! readable for all

		//! note: that the following constants include all states beneath it (e.g. PRIVATE_MASK includes PUBLIC,ALLIED,INLOS,...)
		RULESPARAMLOS_PRIVATE_MASK = 31,
		RULESPARAMLOS_ALLIED_MASK  = 30,
		RULESPARAMLOS_INLOS_MASK   = 28,
		RULESPARAMLOS_INRADAR_MASK = 24,
		RULESPARAMLOS_PUBLIC_MASK  = 16
	};

	struct Param {
		CR_DECLARE_STRUCT(Param)

		Param() : los(RULESPARAMLOS_PRIVATE),valueInt(0.0f) {};

		int   los;
		float valueInt;
		std::string valueString;
	};

	typedef std::unordered_map<std::string, Param> Params;
}

#endif // LUA_RULESPARAMS_H
