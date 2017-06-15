/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NetLog.h"

#if (!defined(DEDICATED) && !defined(UNITSYNC) && !defined(BUILDING_AI) && !defined(UNIT_TEST))

#include "Net/Protocol/NetProtocol.h"

void CLIENT_NETLOG(uint8_t myPlayerNum, const std::string& msg)
{
	clientNet->Send(CBaseNetProtocol::Get().SendLogMsg(myPlayerNum, msg));

}

#else

#include "System/Log/ILog.h"
void CLIENT_NETLOG(uint8_t myPlayerNum, const std::string& msg)
{
	LOG("NETLOG %d %s", myPlayerNum, msg.c_str());
}
#endif



