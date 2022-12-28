/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BuildType.h"

namespace BuildType {
bool IsHeadless()
{
#ifdef HEADLESS
	return true;
#else
	return false;
#endif
}

bool IsUnitsync()
{
#ifdef UNITSYNC
	return true;
#else
	return false;
#endif
}

bool IsDedicated()
{
#ifdef DEDICATED
	return true;
#else
	return false;
#endif
}
}
