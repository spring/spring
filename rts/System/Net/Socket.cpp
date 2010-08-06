/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef _MSC_VER
#	include "StdAfx.h"
#elif defined(_WIN32)
#	include <windows.h>
#endif

#include "Socket.h"

#include <boost/system/error_code.hpp>
#include "lib/streflop/streflop_cond.h"

#include "LogOutput.h"

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
	// connection reset can happen when host didn't start up before the client wants to connect
	if (!err || err.value() == connection_reset)
	{
		return false;
	}
	else
	{
		LogObject() << "Network error " << err.value() << ": " << err.message();
		return true;
	}
}

boost::asio::ip::udp::endpoint ResolveAddr(const std::string& ip, int port)
{
	using namespace boost::asio;
	boost::system::error_code err;
	ip::address tempAddr = ip::address::from_string(ip, err);
#ifdef STREFLOP_H
	//! (date of note: 08/05/10)
	//! something in from_string() is invalidating the FPU flags
	//! tested on win2k and linux (not happening there)
	streflop_init<streflop::Simple>();
#endif
	if (err)
	{
		// error, maybe a hostname?
		ip::udp::resolver resolver(netcode::netservice);
		std::ostringstream portbuf;
		portbuf << port;
		ip::udp::resolver::query query(ip, portbuf.str());
		ip::udp::resolver::iterator iter = resolver.resolve(query);
		tempAddr = iter->endpoint().address();
	}

	return ip::udp::endpoint(tempAddr, port);
}

} // namespace netcode

