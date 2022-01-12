/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WINVERSION_H
#define WINVERSION_H

#include <string>

namespace windows {
	std::string GetDisplayString(bool getName, bool getVersion, bool getExtra);
	std::string GetHardwareString();
};

#endif
