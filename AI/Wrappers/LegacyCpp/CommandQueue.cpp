/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommandQueue.h"

#include "System/creg/creg_cond.h"

#ifdef USING_CREG
creg::Class* springLegacyAI::CCommandQueue::GetClass() const {
	return NULL;
}
#endif

