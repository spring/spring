/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BUILD_TYPE_H
#define BUILD_TYPE_H

namespace BuildType
{
	/// Returns true if this build is a "HEADLESS" build
	extern bool IsHeadless();

	/// Returns true if this build is a "UNITSYNC" build
	extern bool IsUnitsync();

	/// Returns true if this build is a "DEDICATED" build
	extern bool IsDedicated();
}

#endif // BUILD_TYPE_H
