/** @file GameVersion.cpp
	@brief Defines the current version string.
	Take special care when moving this file, the Spring buildbot refers to this
	file to append the version string with the SVN revision number.
*/
#include "StdAfx.h"
#include "GameVersion.h"

#include <cstring>

// IMPORTANT NOTE: external systems sed -i this file so DO NOT CHANGE without
// major thought in advance, and deliberation with bibim and tvo/Tobi!

namespace SpringVersion
{
	
const char* const Major = "0.79+";
const char* const Minor = "0";
const char* const Patchset = "0";
const char* const Additional = "";

/** Build date and time. */
const char* const BuildTime = __DATE__ " " __TIME__;

std::string Get()
{
	return std::string(Major) + "." + Minor;
}

std::string GetFull()
{
	static const std::string full(Get() + "." + Patchset + ((std::strlen(Additional) >0) ? (std::string(" (") + Additional + ")") : ""));
	return full;
}

}
