/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_SCRIPT_LOG_H
#define UNIT_SCRIPT_LOG_H

#define LOG_SECTION_UNIT_SCRIPT "UnitScript"

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_UNIT_SCRIPT

#include "System/Log/ILog.h"

LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_UNIT_SCRIPT)

#endif // UNIT_SCRIPT_LOG_H
