#ifndef AUTOHOSTINTERFACE 
#define AUTOHOSTINTERFACE

#include <string>

#include "Game/Server/ServerLog.h"

namespace netcode {
	class UDPConnectedSocket;
}

/**
@brief Class to communicate with an autohost (or similar) using UDP over loopback
@author Karl-Robert Ernst
*/
class AutohostInterface : public ServerLog
{
public:
	typedef unsigned char uchar;
	
	/**
	@brief Connects to a port on localhost
	@param localport port to use by this class
	@param remoteport the port of the autohost
	*/
	AutohostInterface(int localport, int remoteport);
	virtual ~AutohostInterface();
	
	void SendStart() const;
	void SendQuit() const;
	void SendStartPlaying() const;
	void SendGameOver() const;
	
	void SendPlayerJoined(uchar playerNum, const std::string& name) const;
	void SendPlayerLeft(uchar playerNum, uchar reason) const;
	void SendPlayerReady(uchar playerNum, uchar readyState) const;
	void SendPlayerChat(uchar playerNum, const std::string& msg) const;
	void SendPlayerDefeated(uchar playerNum) const;
	
	virtual void Message(const std::string& message);
	virtual void Warning(const std::string& message);
	
	/**
	@brief Recieve a chat message from the autohost
	There should be only 1 message per UDP-Packet, and it will use the hosts playernumber to inject this message
	*/
	std::string GetChatMessage() const;
	
private:
	netcode::UDPConnectedSocket* autohost;
};

#endif
