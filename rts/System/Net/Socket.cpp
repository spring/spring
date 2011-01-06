/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef _MSC_VER
#	include "StdAfx.h"
#elif defined(_WIN32)
#	include <windows.h>
#endif

#include "Socket.h"

#include <boost/system/error_code.hpp>
#include "lib/streflop/streflop_cond.h"

#include "System/LogOutput.h"

namespace netcode
{
#if BOOST_VERSION < 103600
using namespace boost::system::posix_error;
#else
using namespace boost::system::errc;
#endif

boost::asio::io_service netservice;

bool CheckErrorCode(boost::system::error_code& err)
{
	// connection reset can happen when host did not start up
	// before the client wants to connect
	if (!err || err.value() == connection_reset) {
		return false;
	} else {
		LogObject() << "Network error " << err.value() << ": " << err.message();
		return true;
	}
}

boost::asio::ip::udp::endpoint ResolveAddr(const std::string& host, int port)
{
	using namespace boost::asio;
	boost::system::error_code err;
	ip::address tempAddr = WrapIP(host, &err);
	if (err) {
		// error, maybe a hostname?
		ip::udp::resolver resolver(netcode::netservice);
		std::ostringstream portbuf;
		portbuf << port;
		ip::udp::resolver::query query(host, portbuf.str());
		// TODO: check if unsync problem exists here too (see WrapResolve below)
		ip::udp::resolver::iterator iter = resolver.resolve(query);
		tempAddr = iter->endpoint().address();
	}

	return ip::udp::endpoint(tempAddr, port);
}

bool IsLoopbackAddress(const boost::asio::ip::address& addr) {

	if (addr.is_v6()) {
		return addr.to_v6().is_loopback();
	} else {
		return (addr.to_v4() == boost::asio::ip::address_v4::loopback());
	}
}

boost::asio::ip::address WrapIP(const std::string& ip,
		boost::system::error_code* err)
{
	boost::asio::ip::address addr;

	if (err == NULL) {
		addr = boost::asio::ip::address::from_string(ip);
	} else {
		addr = boost::asio::ip::address::from_string(ip, *err);
	}
#ifdef STREFLOP_H
	//! (date of note: 08/05/10)
	//! something in from_string() is invalidating the FPU flags
	//! tested on win2k and linux (not happening there)
	streflop_init<streflop::Simple>();
#endif

	return addr;
}

boost::asio::ip::tcp::resolver::iterator WrapResolve(
		boost::asio::ip::tcp::resolver& resolver,
		boost::asio::ip::tcp::resolver::query& query,
		boost::system::error_code* err)
{
	boost::asio::ip::tcp::resolver::iterator resolveIt;

	if (err == NULL) {
		resolveIt = resolver.resolve(query);
	} else {
		resolveIt = resolver.resolve(query, *err);
	}
#ifdef STREFLOP_H
	//! (date of note: 08/22/10)
	//! something in resolve() is invalidating the FPU flags
	streflop_init<streflop::Simple>();
#endif

	return resolveIt;
}

} // namespace netcode

