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

CR_BIND(CommandDescription, )
CR_REG_METADATA(CommandDescription, (
	CR_MEMBER(id),
	CR_MEMBER(type),
	CR_MEMBER(name),
	CR_MEMBER(action),
	CR_MEMBER(iconname),
	CR_MEMBER(mouseicon),
	CR_MEMBER(tooltip),
	CR_MEMBER(hidden),
	CR_MEMBER(disabled),
	CR_MEMBER(showUnique),
	CR_MEMBER(onlyTexture),
	CR_MEMBER(params),
	CR_RESERVED(32)
))
