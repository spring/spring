/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// IMPORTANT NOTE: external systems sed -i this file, so DO NOT CHANGE without
// major thought in advance, and deliberation with bibim and tvo/Tobi!

#include "GameVersion.h"

#include <cstring>

/**
 * @brief Defines the current version string.
 * Take special care when moving this file.
 * The build-bot refers to this file to append the exact SCM version.
 */
namespace SpringVersion
{
const char* const Major = "0.82+";
const char* const Minor = "4";
const char* const Patchset = "0";
const char* const Additional = "" // Build-Bot will write in here before compiling

#if !defined GV_ADD_SPACE
	// Build-Bot should set this to " " if it put something into the above line
	#define GV_ADD_SPACE ""
#endif

#if defined DEBUG
	GV_ADD_SPACE "Debug"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined USE_MMGR
	GV_ADD_SPACE "mmgr"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined USE_GML
	GV_ADD_SPACE "MT"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif
#if defined USE_GML_SIM
	"-Sim"
#endif
#if defined USE_GML_DEBUG
	"+Debug"
#endif

#if defined TRACE_SYNC
	GV_ADD_SPACE "Sync-Trace"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined SYNCDEBUG
	GV_ADD_SPACE "Sync-Debug"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if !defined SYNCCHECK
	GV_ADD_SPACE "Sync-Check-Disabled"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined HEADLESS
	GV_ADD_SPACE "Headless"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif

#if defined UNITSYNC
	GV_ADD_SPACE "Unitsync"
	#undef  GV_ADD_SPACE
	#define GV_ADD_SPACE " "
#endif
	;

/** Build date and time. */
const char* const BuildTime = __DATE__ " " __TIME__;

std::string Get()
{
	return std::string(Major) + "." + Minor;
}

std::string GetFull()
{
	static const std::string full(Get() + "." + Patchset + ((std::strlen(Additional) > 0) ? (std::string(" (") + Additional + ")") : ""));

	return full;
}

}
