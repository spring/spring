/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SOUND_LOG_H_
#define _SOUND_LOG_H_

#define LOG_SECTION_SOUND "Sound"

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_SOUND

#include "System/Log/ILog.h"

LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_SOUND)

#endif // _SOUND_LOG_H_
