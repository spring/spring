/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_PARTICIPANT_H
#define _GAME_PARTICIPANT_H

#include <memory>

#include "Game/Players/PlayerBase.h"
#include "Game/Players/PlayerStatistics.h"
#include "System/Net/LoopbackConnection.h"
#include "System/UnorderedMap.hpp"

namespace netcode
{
	class CConnection;
	class RawPacket;
}

class GameParticipant : public PlayerBase
{
public:
	GameParticipant();

	void SendData(std::shared_ptr<const netcode::RawPacket> packet);
	void Connected(std::shared_ptr<netcode::CConnection> link, bool local);
	void Kill(const std::string& reason, const bool flush = false);

	GameParticipant& operator=(const PlayerBase& base) { PlayerBase::operator=(base); return *this; };

public:
	int id = -1;
	int lastFrameResponse = 0;

	enum State {
		UNCONNECTED,
		CONNECTED,
		INGAME,
		DISCONNECTED
	};
	State myState = UNCONNECTED;

	bool isLocal = false;
	bool isReconn = false;
	bool isMidgameJoin = false;

	PlayerStatistics lastStats;

	struct ClientLinkData {
		ClientLinkData(bool connect = true) {
			if (connect)
				link.reset(new netcode::CLoopbackConnection());
		}

		std::shared_ptr<netcode::CConnection> link;

		int bandwidthUsage = 0;
		int numPacketsSent = 0;
	};

	std::shared_ptr<netcode::CConnection> clientLink;
	spring::unordered_map<uint8_t, ClientLinkData> aiClientLinks;

	#ifdef SYNCCHECK
	spring::unordered_map<int, unsigned int> syncResponse; // syncResponse[frameNum] = checksum
	#endif
};

#endif // _GAME_PARTICIPANT_H
