/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/type2.h"
#include "System/creg/creg_cond.h"

CR_BIND_TEMPLATE(int2, )
CR_REG_METADATA(int2, (CR_MEMBER(x), CR_MEMBER(y)))

CR_BIND_TEMPLATE(float2, )
CR_REG_METADATA(float2, (CR_MEMBER(x), CR_MEMBER(y)))

CR_BIND_TEMPLATE(short2, )
CR_REG_METADATA(short2, (CR_MEMBER(x), CR_MEMBER(y)))

CR_BIND_TEMPLATE(ushort2, )
CR_REG_METADATA(ushort2, (CR_MEMBER(x), CR_MEMBER(y)))
