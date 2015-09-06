/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameParticipant.h"

#include "Net/Protocol/BaseNetProtocol.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Net/Connection.h"
#include "System/Misc/SpringTime.h"

GameParticipant::GameParticipant()
: id(-1)
, myState(UNCONNECTED)
, lastFrameResponse(0)
, speedControl(0)
, isLocal(false)
, isReconn(false)
, isMidgameJoin(false)
{
	linkData[MAX_AIS] = PlayerLinkData(false);
}

void GameParticipant::SendData(boost::shared_ptr<const netcode::RawPacket> packet)
{
	if (link)
		link->SendData(packet);
}

void GameParticipant::Connected(boost::shared_ptr<netcode::CConnection> _link, bool local)
{
	link = _link;
	linkData[MAX_AIS].link.reset(new netcode::CLoopbackConnection());
	isLocal = local;
	myState = CONNECTED;
	lastFrameResponse = 0;
}

void GameParticipant::Kill(const std::string& reason, const bool flush)
{
	if (link) {
		link->SendData(CBaseNetProtocol::Get().SendQuit(reason));

		// make sure the Flush() performed by Close() has effect (forced flushes are undesirable)
		// it will cause a slight lag in the game server during kick, but not a big deal
		if (flush)
			spring_sleep(spring_msecs(1000));

		link->Close(flush);
		link.reset();
	}
	linkData[MAX_AIS].link.reset();
#ifdef SYNCCHECK
	syncResponse.clear();
#endif
	myState = DISCONNECTED;
}

