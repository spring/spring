/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaRulesParams.h"

using namespace LuaRulesParams;

CR_BIND(Param,)
CR_REG_METADATA(Param, (
	CR_MEMBER(los),
	CR_MEMBER(valueInt),
	CR_MEMBER(valueString)
))
