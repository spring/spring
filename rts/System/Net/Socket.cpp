/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Socket.h"

#include <boost/system/error_code.hpp>
#include "lib/streflop/streflop_cond.h"

#include "System/Log/ILog.h"
#include "System/Util.h"


namespace netcode
{
using namespace boost::system::errc;

boost::asio::io_service netservice;

bool CheckErrorCode(boost::system::error_code& err)
{
	// connection reset can happen when host did not start up
	// before the client wants to connect
	if (!err || err.value() == connection_reset || 
		err.value() == resource_unavailable_try_again) { // this should only ever happen with async sockets, but testing indicates it happens anyway...
		return false;
	} else {
		LOG_L(L_WARNING, "Network error %i: %s", err.value(),
				err.message().c_str());
		return true;
	}
}

boost::asio::ip::udp::endpoint ResolveAddr(const std::string& host, int port, boost::system::error_code* err)
{
	assert(err);
	using namespace boost::asio;
	ip::address tempAddr = WrapIP(host, err);
	if (!*err)
		return ip::udp::endpoint(tempAddr, port);

	auto errBuf = *err; // WrapResolve() might clear err
	boost::asio::io_service io_service;
	ip::udp::resolver resolver(io_service);
	ip::udp::resolver::query query(host, IntToString(port));
	auto iter = WrapResolve(resolver, query, err);
	ip::udp::resolver::iterator end;
	if (!*err && iter != end) {
		return *iter;
	}

	if (!*err) *err = errBuf;
	return ip::udp::endpoint(tempAddr, 0);
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
//#ifdef STREFLOP_H
	// (date of note: 08/05/10)
	// something in from_string() is invalidating the FPU flags
	// tested on win2k and linux (not happening there)
	streflop::streflop_init<streflop::Simple>();
//#endif

	return addr;
}

boost::asio::ip::udp::resolver::iterator WrapResolve(
		boost::asio::ip::udp::resolver& resolver,
		boost::asio::ip::udp::resolver::query& query,
		boost::system::error_code* err)
{
	boost::asio::ip::udp::resolver::iterator resolveIt;

	if (err == NULL) {
		resolveIt = resolver.resolve(query);
	} else {
		resolveIt = resolver.resolve(query, *err);
	}
//#ifdef STREFLOP_H
	// (date of note: 08/22/10)
	// something in resolve() is invalidating the FPU flags
	streflop::streflop_init<streflop::Simple>();
//#endif

	return resolveIt;
}


boost::asio::ip::address GetAnyAddress(const bool IPv6)
{
	if (IPv6) {
		return boost::asio::ip::address_v6::any();
	}
	return boost::asio::ip::address_v4::any();
}


} // namespace netcode

