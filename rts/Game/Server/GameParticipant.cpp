/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameParticipant.h"

#include "Net/Connection.h"
#include "BaseNetProtocol.h"

GameParticipant::GameParticipant()
: myState(UNCONNECTED)
, lastFrameResponse(0)
, speedControl(0)
, luaDrawTime(0)
, isLocal(false)
, isReconn(false)
, isMidgameJoin(false)
, bandwidthUsage(0)
{
}

void GameParticipant::SendData(boost::shared_ptr<const netcode::RawPacket> packet)
{
	if (link)
		link->SendData(packet);
}

void GameParticipant::Connected(boost::shared_ptr<netcode::CConnection> _link, bool local)
{
	link = _link;
	isLocal = local;
	myState = CONNECTED;
}

void GameParticipant::Kill(const std::string& reason)
{
	if (link)
	{
		link->SendData(CBaseNetProtocol::Get().SendQuit(reason));
		link.reset();
	}
#ifdef SYNCCHECK
	syncResponse.clear();
#endif
	myState = DISCONNECTED;
}

