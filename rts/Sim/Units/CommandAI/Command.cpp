/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

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

#if USE_SAFE_VECTOR
CR_BIND_TEMPLATE(safe_vector<float>, );

const float safe_vector<float>::defval = 0.0f;
float safe_vector<float>::dummy = 0.0f;
bool safe_vector<float>::firsterror = true;

const float& safe_vector<float>::safe_element() const {
	if(firsterror) {
		firsterror = false;
		logOutput.Print("ERROR: Vector read attempt out of bounds!");
		CrashHandler::OutputStacktrace();
	}
	return defval;
}
float& safe_vector<float>::safe_element() {
	if(firsterror) {
		firsterror = false;
		logOutput.Print("ERROR: Vector assignment attempt out of bounds!");
		CrashHandler::OutputStacktrace();
	}
	return dummy;
}

#endif
