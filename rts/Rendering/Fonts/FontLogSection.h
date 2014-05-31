/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FONT_LOG_H_
#define _FONT_LOG_H_

#define LOG_SECTION_FONT "Font"

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_FONT

#include "System/Log/ILog.h"

LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_FONT)

#endif // _FONT_LOG_H_
