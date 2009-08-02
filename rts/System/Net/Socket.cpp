#ifdef _MSC_VER
#	include "StdAfx.h"
#elif defined(_WIN32)
#	include <windows.h>
#endif

#include "Socket.h"

#include <boost/system/error_code.hpp>

#include "LogOutput.h"

namespace netcode
{
using namespace boost::system::errc;

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

} // namespace netcode

