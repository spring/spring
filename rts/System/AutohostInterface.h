#ifndef AUTOHOSTINTERFACE 
#define AUTOHOSTINTERFACE

#include <string>
#include <boost/cstdint.hpp>
#include <boost/asio/ip/udp.hpp>

/**
@brief Class to communicate with an autohost (or similar) using UDP over loopback
@author Karl-Robert Ernst
*/
class AutohostInterface
{
public:
	typedef unsigned char uchar;
	
	/**
	@brief Connects to a port on localhost
	@param localport port to use by this class
	@param remoteport the port of the autohost
	*/
	AutohostInterface(int remoteport);
	virtual ~AutohostInterface();
	
	void SendStart();
	void SendQuit();
	void SendStartPlaying();
	void SendGameOver();
	
	void SendPlayerJoined(uchar playerNum, const std::string& name);
	void SendPlayerLeft(uchar playerNum, uchar reason);
	void SendPlayerReady(uchar playerNum, uchar readyState);
	void SendPlayerChat(uchar playerNum, uchar destination, const std::string& msg);
	void SendPlayerDefeated(uchar playerNum);
	
	void Message(const std::string& message);
	void Warning(const std::string& message);
	
	void SendLuaMsg(const boost::uint8_t* msg, size_t msgSize);
	void Send(const boost::uint8_t* msg, size_t msgSize);
	
	/**
	@brief Receive a chat message from the autohost
	There should be only 1 message per UDP-Packet, and it will use the hosts playernumber to inject this message
	*/
	std::string GetChatMessage();
	
private:
	boost::asio::ip::udp::socket autohost;
};

#endif
