/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_PARTICIPANT_H
#define _GAME_PARTICIPANT_H

#include <boost/shared_ptr.hpp>

#include "Game/PlayerBase.h"
#include "Game/PlayerStatistics.h"
#include "System/Net/LoopbackConnection.h"

namespace netcode
{
	class CConnection;
	class RawPacket;
}

class GameParticipant : public PlayerBase
{
public:
	GameParticipant();
	void SendData(boost::shared_ptr<const netcode::RawPacket> packet);

	void Connected(boost::shared_ptr<netcode::CConnection> link, bool local);
	void Kill(const std::string& reason, const bool flush = false);

	GameParticipant& operator=(const PlayerBase& base) { PlayerBase::operator=(base); return *this; };

	enum State
	{
		UNCONNECTED,
		CONNECTED,
		INGAME,
		DISCONNECTED
	};
	State myState;

	int lastFrameResponse;
	int speedControl;

	bool isLocal;
	bool isReconn;
	bool isMidgameJoin;
	boost::shared_ptr<netcode::CConnection> link;
	PlayerStatistics lastStats;

	struct PlayerLinkData {
		PlayerLinkData(bool connect = true) : bandwidthUsage(0) { if (connect) link.reset(new netcode::CLoopbackConnection()); }
		boost::shared_ptr<netcode::CConnection> link;
		int bandwidthUsage;
	};
	std::map<unsigned char, PlayerLinkData> linkData;

#ifdef SYNCCHECK
	std::map<int, unsigned> syncResponse; // syncResponse[frameNum] = checksum
#endif
};

#endif // _GAME_PARTICIPANT_H
