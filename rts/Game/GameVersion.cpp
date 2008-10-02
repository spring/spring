/** @file GameVersion.cpp
	@brief Defines the current version string.
	Take special care when moving this file, the Spring buildbot refers to this
	file to append the version string with the SVN revision number.
*/
#include "StdAfx.h"

#include "GameVersion.h"

/** The game version. */
const char* const VERSION_STRING = "0.76b1+";
