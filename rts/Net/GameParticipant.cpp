/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameParticipant.h"

#include "Net/Protocol/BaseNetProtocol.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Net/Connection.h"
#include "System/Misc/SpringTime.h"

GameParticipant::GameParticipant()
{
	aiClientLinks[MAX_AIS] = ClientLinkData(false);
}

void GameParticipant::SendData(std::shared_ptr<const netcode::RawPacket> packet)
{
	if (clientLink != nullptr)
		clientLink->SendData(packet);
}

void GameParticipant::Connected(std::shared_ptr<netcode::CConnection> _link, bool local)
{
	clientLink = _link;
	aiClientLinks[MAX_AIS].link.reset(new netcode::CLoopbackConnection());

	isLocal = local;
	myState = CONNECTED;
	lastFrameResponse = 0;
}

void GameParticipant::Kill(const std::string& reason, const bool flush)
{
	if (clientLink != nullptr) {
		clientLink->SendData(CBaseNetProtocol::Get().SendQuit(reason));

		// make sure the Flush() performed by Close() has effect (forced flushes are undesirable)
		// it will cause a slight lag in the game server during kick, but not a big deal
		if (flush)
			spring_sleep(spring_msecs(1000));

		clientLink->Close(flush);
		clientLink.reset();
	}

	aiClientLinks[MAX_AIS].link.reset();
#ifdef SYNCCHECK
	syncResponse.clear();
#endif

	myState = DISCONNECTED;
}

