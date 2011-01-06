/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOCKET_H
#define SOCKET_H

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ip/tcp.hpp>


namespace netcode
{

extern boost::asio::io_service netservice;

/**
 * Check if a network error occured and eventually log it.
 * @returns true if a network error occured, false otherwise
 */
bool CheckErrorCode(boost::system::error_code& err);

/**
 * Resolves a host name or IP plus port number into a boost UDP end-point,
 * eventually resolving the host, if it is specified as name.
 * @param host name or IP
 */
boost::asio::ip::udp::endpoint ResolveAddr(const std::string& host, int port);

/**
 * Evaluates if an address is a loopback one or not.
 * In IP v4 it is "127.*.*.*", in IP v6 "::1".
 */
bool IsLoopbackAddress(const boost::asio::ip::address& addr);

/**
 * Encapsulates the ip::address::from_string(str) function,
 * for sync relevant reasons.
 */
boost::asio::ip::address WrapIP(const std::string& ip,
		boost::system::error_code* err = NULL);

/**
 * Encapsulates the ip::tcp::resolver::resolve(query) function,
 * for sync relevant reasons.
 */
boost::asio::ip::tcp::resolver::iterator WrapResolve(
		boost::asio::ip::tcp::resolver& resolver,
		boost::asio::ip::tcp::resolver::query& query,
		boost::system::error_code* err = NULL);

} // namespace netcode

#endif // SOCKET_H
