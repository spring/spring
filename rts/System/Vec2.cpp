/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Vec2.h"

#include "creg/creg_cond.h"

CR_BIND(int2, );
CR_REG_METADATA(int2, (CR_MEMBER(x), CR_MEMBER(y)));

CR_BIND(float2, );
CR_REG_METADATA(float2, (CR_MEMBER(x), CR_MEMBER(y)));
