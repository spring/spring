/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOCKET_H
#define SOCKET_H

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>


namespace netcode
{

extern boost::asio::io_service netservice;
bool CheckErrorCode(boost::system::error_code&);
boost::asio::ip::udp::endpoint ResolveAddr(const std::string& ip, int port);

} // namespace netcode

#endif
