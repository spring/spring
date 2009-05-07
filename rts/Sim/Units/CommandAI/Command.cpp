#include "StdAfx.h"
// command.cpp: implementation of the CCommandDescription class.
//
//////////////////////////////////////////////////////////////////////

#include "Command.h"


CR_BIND(Command, );
CR_REG_METADATA(Command, (
				CR_MEMBER(id),
				CR_MEMBER(options),
				CR_MEMBER(params),
				CR_MEMBER(tag),
				CR_MEMBER(timeOut),
				CR_RESERVED(16)
				));


CR_BIND(CommandDescription, );
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
				));


/******************************************************************************/

