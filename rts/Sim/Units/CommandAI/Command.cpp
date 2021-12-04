/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Command.h"

CR_BIND(Command, )
CR_REG_METADATA(Command, (
	CR_MEMBER(id),
	CR_MEMBER(aiCommandId),

	CR_MEMBER(timeOut),
	CR_MEMBER(tag),
	CR_MEMBER(options),

	CR_MEMBER(params)
))
