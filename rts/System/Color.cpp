/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Color.h"

CR_BIND(SColor, (0,0,0,0))
CR_REG_METADATA(SColor,(
	CR_MEMBER(i)
))

const SColor SColor::Zero = SColor(0  , 0  , 0  , 0  );
const SColor SColor::One  = SColor(255, 255, 255, 255);
