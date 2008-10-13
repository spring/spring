/** @file GameVersion.cpp
	@brief Defines the current version string.
	Take special care when moving this file, the Spring buildbot refers to this
	file to append the version string with the SVN revision number.
*/
#include "StdAfx.h"
#include "GameVersion.h"

// IMPORTANT NOTE: external systems sed -i this file so DO NOT CHANGE without
// major thought in advance, and deliberation with bibim and tvo/Tobi!

/** The game version as it's returned by unitsync and Spring on commandline.
This version string is the one that's used to check sync. */
const char* const VERSION_STRING = "0.77b3+";

/** The game version as it's printed to infolog, for e.g. stacktrace translator. */
const char* const VERSION_STRING_DETAILED = "0.77b3+";

/** Build date and time. */
const char* const BUILD_DATETIME = __DATE__ " " __TIME__;
