/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NETWORK_EXCEPTION_H
#define NETWORK_EXCEPTION_H

#include <stdexcept>

namespace netcode
{

/**
 * @brief network_error
 * thrown when network error occured
 */
class network_error : public std::runtime_error
{
public:
	network_error(const std::string& msg) :	std::runtime_error(msg) {}
};

} // namespace netcode

#endif // NETWORK_EXCEPTION_H
