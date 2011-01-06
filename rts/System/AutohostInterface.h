/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AUTOHOST_INTERFACE_H
#define AUTOHOST_INTERFACE_H

#include <string>
#include <boost/cstdint.hpp>
#include <boost/asio/ip/udp.hpp>

/**
 * API for engine <-> autohost (or similar) communication, using UDP over
 * loopback.
 */
class AutohostInterface
{
public:
	typedef unsigned char uchar;

	/**
	 * @brief Connects to a port on localhost
	 * @param remoteHost name or IP of the autohost to connect to
	 * @param remotePort the port where the autohost runs its
	 *   communication-with-engine service
	 */
	AutohostInterface(const std::string& remoteHost, int remotePort);
	virtual ~AutohostInterface();

	void SendStart();
	void SendQuit();
	void SendStartPlaying();
	void SendGameOver(uchar playerNum, const std::vector<uchar>& winningAllyTeams);

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
	 * @brief Receive a chat message from the autohost
	 * There should be only 1 message per UDP-Packet, and it will use the hosts
	 * playernumber to inject this message
	 */
	std::string GetChatMessage();

private:
	void Send(boost::asio::mutable_buffers_1 sendBuffer);

	boost::asio::ip::udp::socket autohost;
};

#endif // AUTOHOST_INTERFACE_H
