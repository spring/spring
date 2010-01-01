#ifndef GAMEPARTICIPANT_H
#define GAMEPARTICIPANT_H

#include <boost/shared_ptr.hpp>

#include "Game/PlayerBase.h"
#include "Game/PlayerStatistics.h"

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
	void Kill(const std::string& reason);

	void operator=(const PlayerBase& base) { PlayerBase::operator=(base); };

	enum State
	{
		UNCONNECTED,
		CONNECTED,
		INGAME,
		DISCONNECTED
	};
	State myState;
	
	int lastFrameResponse;

	bool isLocal;
	boost::shared_ptr<netcode::CConnection> link;
	PlayerStatistics lastStats;
#ifdef SYNCCHECK
	std::map<int, unsigned> syncResponse; // syncResponse[frameNum] = checksum
#endif
};

#endif // GAMEPARTICIPANT_H
