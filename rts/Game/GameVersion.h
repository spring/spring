/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GAME_VERSION_H
#define GAME_VERSION_H

#include <string>

namespace SpringVersion
{
	/// major revision number (e.g. 0.77)
	extern const char* const Major;

	/// minor revision / bugfix which breaks sync between clients
	extern const char* const Minor;

	/**
	 * @brief bug fixes which preserves sync between clients
	 * Clients with the same Major.Minor can still play together.
	 * Demos should also be compatible between patch-sets.
	 */
	extern const char* const Patchset;

	/// additional information (compiler flags, SCM revision etc.)
	extern const char* const Additional;

	/// time of build
	extern const char* const BuildTime;

	/**
	 * @return "Major.Minor"
	 * @see Major
	 * @see Minor
	 */
	extern std::string Get();

	/**
	 * @return "Major.Minor.Patchset (Additional)"
	 * @see Major
	 * @see Minor
	 * @see Patchset
	 * @see Additional
	 */
	extern std::string GetFull();
};

#endif // GAME_VERSION_H
