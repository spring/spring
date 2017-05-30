/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_FORMAT_H
#define SPRING_FORMAT_H

#include "MainDefines.h"
#include <string>

namespace spring {
	template<class... Args>
	std::string format(const char* fmt, const Args&... args) {
		char buff[4096];
		const int len = SNPRINTF(buff, 4096, fmt, args...);
		if ((len > 0) && (len < 4096))
			return std::string(buff, len);

		return std::string();
	}
	template<class... Args>
	std::string format(const std::string& fmt, const Args&... args) {
		return format(fmt.c_str(), args...);
	}
}

#endif //SPRING_FORMAT_H
