/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _NAME_RESOLVER_H
#define _NAME_RESOLVER_H

#include <string>

namespace NameResolver {
	std::string GetGame(const std::string& lazyName);
	std::string GetMap(const std::string& lazyName);
}

#endif // _NAME_RESOLVER_H