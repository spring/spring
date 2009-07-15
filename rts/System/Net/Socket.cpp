#ifdef _MSC_VER
#	include "StdAfx.h"
#elif defined(_WIN32)
#	include <windows.h>
#endif

#include "Socket.h"

#ifndef _MSC_VER
#include "StdAfx.h"
#endif

namespace netcode
{

boost::asio::io_service netservice;

} // namespace netcode

