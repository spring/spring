/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NETWORK_EXCEPTION
#define NETWORK_EXCEPTION

#include <stdexcept>

namespace netcode
{

/**
 * network_error
 * thrown when network error occured
 */
class network_error : public std::runtime_error
{
public:
	network_error(const std::string& msg) :	std::runtime_error(msg) {};
};

}

#endif
