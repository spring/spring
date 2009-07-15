#include "GameParticipant.h"

#include "Net/Connection.h"
#include "BaseNetProtocol.h"

GameParticipant::GameParticipant()
: myState(UNCONNECTED)
, cpuUsage (0.0f)
, ping (0)
, lastKeyframeResponse(0)
, isLocal(false)
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

void GameParticipant::Kill()
{
	if (link)
	{
		link->SendData(CBaseNetProtocol::Get().SendQuit());
		link.reset();
	}
	myState = DISCONNECTED;
}
