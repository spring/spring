/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AUTOHOST_INTERFACE_H
#define AUTOHOST_INTERFACE_H

#include <string>
#include <cinttypes>
#include <asio/ip/udp.hpp>

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
	 * @param remoteIP IP of the autohost to connect to
	 * @param remotePort the port where the autohost runs its
	 *   communication-with-engine service
	 * @param localIP the local IP to use in the connection,
	 *   use "" to use the any IP
	 * @param localPort the local port to use in the connection,
	 *   use 0 for OS-select
	 */
	AutohostInterface(const std::string& remoteIP, int remotePort,
			const std::string& localIP = "", int localPort = 0);
	virtual ~AutohostInterface() {}

	bool IsInitialized() const { return initialized; }

	void SendStart();
	void SendQuit();
	void SendStartPlaying(const unsigned char* gameID, const std::string& demoName);
	void SendGameOver(uchar playerNum, const std::vector<uchar>& winningAllyTeams);

	void SendPlayerJoined(uchar playerNum, const std::string& name);
	void SendPlayerLeft(uchar playerNum, uchar reason);
	void SendPlayerReady(uchar playerNum, uchar readyState);
	void SendPlayerChat(uchar playerNum, uchar destination, const std::string& msg);
	void SendPlayerDefeated(uchar playerNum);

	void Message(const std::string& message);
	void Warning(const std::string& message);

	void SendLuaMsg(const std::uint8_t* msg, size_t msgSize);
	void Send(const std::uint8_t* msg, size_t msgSize);

	/**
	 * @brief Receive a chat message from the autohost
	 * There should be only 1 message per UDP-Packet, and it will use the hosts
	 * playernumber to inject this message
	 */
	std::string GetChatMessage();

private:
	void Send(asio::mutable_buffers_1 sendBuffer);

	/**
	 * Tries to bind a socket for communication with a UDP server.
	 * @param socket socket to bind
	 * @param remoteIP IP of the remote host to connect to
	 * @param remotePort the port where the remote host runs its
	 *   UDP service
	 * @param localIP the local IP to use in the connection,
	 *   use "" to use the any IP
	 * @param localPort the local port to use in the connection,
	 *   use 0 for OS-select
	 * @return "" if everything went OK, and error description otherwise
	 */
	static std::string TryBindSocket(asio::ip::udp::socket& socket,
			const std::string& remoteIP, int remotePort,
			const std::string& localIP = "", int localPort = 0);

	asio::ip::udp::socket autohost;
	bool initialized;
};

#endif // AUTOHOST_INTERFACE_H
