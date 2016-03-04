/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommandDescription.h"

CR_BIND(SCommandDescription, )
CR_REG_METADATA(SCommandDescription, (
	CR_MEMBER(id),
	CR_MEMBER(type),

	CR_MEMBER(name),
	CR_MEMBER(action),
	CR_MEMBER(iconname),
	CR_MEMBER(mouseicon),
	CR_MEMBER(tooltip),

	CR_MEMBER(queueing),
	CR_MEMBER(hidden),
	CR_MEMBER(disabled),
	CR_MEMBER(showUnique),
	CR_MEMBER(onlyTexture),
	CR_MEMBER(params)
))
