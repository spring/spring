/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef _MSC_VER
#	include "StdAfx.h"
#elif defined(_WIN32)
#	include <windows.h>
#endif

#include "Net/Socket.h"

#ifndef _MSC_VER
#	include "StdAfx.h"
#endif

#include "AutohostInterface.h"

#include <string.h>
#include <vector>
#include "mmgr.h"
#include "LogOutput.h"

namespace {

/**
 * @enum EVENT Which events can be sent to the autohost
 *   (in brackets: parameters, where uchar means unsigned char and "string"
 *   means plain ascii text)
 */
enum EVENT
{
	/// Server has started ()
	SERVER_STARTED = 0,

	/// Server is about to exit ()
	SERVER_QUIT = 1,

	/// Game starts ()
	SERVER_STARTPLAYING = 2,

	/// Game has ended ()
	SERVER_GAMEOVER = 3,

	/// An information message from server (string message)
	SERVER_MESSAGE = 4,

	/// Server gave out a warning (string warningmessage)
	SERVER_WARNING = 5,

	/// Player has joined the game (uchar playernumber, string name)
	PLAYER_JOINED = 10,

	/**
	 * Player has left (uchar playernumber, uchar reason
	 * (0: lost connection, 1: left, 2: kicked) )
	 */
	PLAYER_LEFT = 11,

	/**
	 * Player has updated its ready-state
	 * (uchar playernumber, uchar state
	 * (0: not ready, 1: ready, 2: state not changed) )
	 */
	PLAYER_READY = 12,

	/**
	 * @brief Player has sent a chat message
	 *   (uchar playernumber, uchar destination, string text)
	 *
	 * Destination can be any of: a playernumber [0-32]
	 * static const int TO_ALLIES = 127;
	 * static const int TO_SPECTATORS = 126;
	 * static const int TO_EVERYONE = 125;
	 * (copied from Game/ChatMessage.h)
	 */
	PLAYER_CHAT = 13,

	/// Player has been defeated (uchar playernumber)
	PLAYER_DEFEATED = 14,

	/**
	 * @brief Message sent by lua script
	 *
	 * (uchar playernumber, uint16_t script, uint8_t mode, uint8_t[X] data)
	 * (X = space left in packet)
	 */
	GAME_LUAMSG = 20,

	/**
	 * @brief team statistics
	 * @see CTeam::Statistics for a reference of how to read them
	 * (uchar teamnumber), CTeam::Statistics(in binary form)
	 */
	GAME_TEAMSTAT = 60
};
}

using namespace boost::asio;

AutohostInterface::AutohostInterface(const std::string& remoteIP, int remotePort, const std::string& localIP, int localPort)
		: autohost(netcode::netservice)
		, initialized(false)
{
	std::string errorMsg = AutohostInterface::TryBindSocket(autohost, remoteIP, remotePort, localIP, localPort);

	if (errorMsg.empty()) {
		initialized = true;
	} else {
		LogObject() << "[AutohostInterface] Error: Failed to open socket: " << errorMsg;
	}
}

std::string AutohostInterface::TryBindSocket(
			boost::asio::ip::udp::socket& socket,
			const std::string& remoteIP, int remotePort,
			const std::string& localIP, int localPort)
{
	std::string errorMsg = "";

	ip::address localAddr;
	ip::address remoteAddr;
	boost::system::error_code err;
	try {
		socket.open(ip::udp::v6(), err); // test IP v6 support
		const bool supportsIPv6 = !err;

		remoteAddr = netcode::WrapIP(remoteIP, &err);
		if (err) {
			throw std::runtime_error("Failed to parse address " + remoteIP + ": " + err.message());
		}

		if (!supportsIPv6 && remoteAddr.is_v6()) {
			throw std::runtime_error("IP v6 not supported, can not use address " + remoteAddr.to_string());
		}

		if (localIP.empty()) {
			// use the "any" address as local "from"
			if (remoteAddr.is_v6()) {
				localAddr = ip::address_v6::any();
			} else {
				if (supportsIPv6) {
					socket.close();
				}
				socket.open(ip::udp::v4());
				localAddr = ip::address_v4::any();
			}
		} else {
			localAddr = netcode::WrapIP(localIP, &err);
			if (err) {
				throw std::runtime_error("Failed to parse local IP " + localIP + ": " + err.message());
			}
			if (localAddr.is_v6() != remoteAddr.is_v6()) {
				throw std::runtime_error("Local IP " + localAddr.to_string() + " and remote IP " + remoteAddr.to_string() + " are IP v4/v6 mixed");
			}
		}

		socket.bind(ip::udp::endpoint(localAddr, localPort));

		boost::asio::socket_base::non_blocking_io command(true);
		socket.io_control(command);

		// A similar, slighly less verbose message is already in GameServer
		//LogObject() << "[AutohostInterface] Connecting (UDP) to IP "
		//		<<  (remoteAddr.is_v6() ? "(v6)" : "(v4)") << " " << remoteAddr
		//		<< " Port " << remotePort;
		socket.connect(ip::udp::endpoint(remoteAddr, remotePort));
	} catch (std::runtime_error& e) { // includes also boost::system::system_error, as it inherits from runtime_error
		socket.close();
		errorMsg = e.what();
		if (errorMsg.empty()) {
			errorMsg = "Unknown problem";
		}
	}

	return errorMsg;
}

AutohostInterface::~AutohostInterface()
{
}

void AutohostInterface::SendStart()
{
	uchar msg = SERVER_STARTED;

	Send(boost::asio::buffer(&msg, sizeof(uchar)));
}

void AutohostInterface::SendQuit()
{
	uchar msg = SERVER_QUIT;

	Send(boost::asio::buffer(&msg, sizeof(uchar)));
}

void AutohostInterface::SendStartPlaying()
{
	uchar msg = SERVER_STARTPLAYING;

	Send(boost::asio::buffer(&msg, sizeof(uchar)));
}

void AutohostInterface::SendGameOver(uchar playerNum, const std::vector<uchar>& winningAllyTeams)
{
	const unsigned char msgsize = 1 + 1 + 1 + (winningAllyTeams.size() * sizeof(uchar));
	std::vector<boost::uint8_t> buffer(msgsize);
	buffer[0] = SERVER_GAMEOVER;
	buffer[1] = msgsize;
	buffer[2] = playerNum;

	for (unsigned int i = 0; i < winningAllyTeams.size(); i++) {
		buffer[3 + i] = winningAllyTeams[i];
	}
	Send(boost::asio::buffer(buffer));
}

void AutohostInterface::SendPlayerJoined(uchar playerNum, const std::string& name)
{
	if (autohost.is_open()) {
		unsigned msgsize = 2 * sizeof(uchar) + name.size();
		std::vector<boost::uint8_t> buffer(msgsize);
		buffer[0] = PLAYER_JOINED;
		buffer[1] = playerNum;
		strncpy((char*)(&buffer[2]), name.c_str(), name.size());

		Send(boost::asio::buffer(buffer));
	}
}

void AutohostInterface::SendPlayerLeft(uchar playerNum, uchar reason)
{
	uchar msg[3] = {PLAYER_LEFT, playerNum, reason};

	Send(boost::asio::buffer(&msg, 3 * sizeof(uchar)));
}

void AutohostInterface::SendPlayerReady(uchar playerNum, uchar readyState)
{
	uchar msg[3] = {PLAYER_READY, playerNum, readyState};

	Send(boost::asio::buffer(&msg, 3 * sizeof(uchar)));
}

void AutohostInterface::SendPlayerChat(uchar playerNum, uchar destination, const std::string& chatmsg)
{
	if (autohost.is_open()) {
		const unsigned msgsize = 3 * sizeof(uchar) + chatmsg.size();
		std::vector<boost::uint8_t> buffer(msgsize);
		buffer[0] = PLAYER_CHAT;
		buffer[1] = playerNum;
		buffer[2] = destination;
		strncpy((char*)(&buffer[3]), chatmsg.c_str(), chatmsg.size());

		Send(boost::asio::buffer(buffer));
	}
}

void AutohostInterface::SendPlayerDefeated(uchar playerNum)
{
	uchar msg[2] = {PLAYER_DEFEATED, playerNum};

	Send(boost::asio::buffer(&msg, 2 * sizeof(uchar)));
}

void AutohostInterface::Message(const std::string& message)
{
	if (autohost.is_open()) {
		const unsigned msgsize = sizeof(uchar) + message.size();
		std::vector<boost::uint8_t> buffer(msgsize);
		buffer[0] = SERVER_MESSAGE;
		strncpy((char*)(&buffer[1]), message.c_str(), message.size());

		Send(boost::asio::buffer(buffer));
	}
}

void AutohostInterface::Warning(const std::string& message)
{
	if (autohost.is_open()) {
		const unsigned msgsize = sizeof(uchar) + message.size();
		std::vector<boost::uint8_t> buffer(msgsize);
		buffer[0] = SERVER_WARNING;
		strncpy((char*)(&buffer[1]), message.c_str(), message.size());

		Send(boost::asio::buffer(buffer));
	}
}

void AutohostInterface::SendLuaMsg(const boost::uint8_t* msg, size_t msgSize)
{
	if (autohost.is_open()) {
		std::vector<boost::uint8_t> buffer(msgSize+1);
		buffer[0] = GAME_LUAMSG;
		std::copy(msg, msg + msgSize, buffer.begin() + 1);

		Send(boost::asio::buffer(buffer));
	}
}

void AutohostInterface::Send(const boost::uint8_t* msg, size_t msgSize)
{
	if (autohost.is_open()) {
		std::vector<boost::uint8_t> buffer(msgSize);
		std::copy(msg, msg + msgSize, buffer.begin());

		Send(boost::asio::buffer(buffer));
	}
}

std::string AutohostInterface::GetChatMessage()
{
	if (autohost.is_open()) {
		size_t bytes_avail = 0;

		if ((bytes_avail = autohost.available()) > 0) {
			std::vector<boost::uint8_t> buffer(bytes_avail+1, 0);
			size_t bytesReceived = autohost.receive(boost::asio::buffer(buffer));
			return std::string((char*)(&buffer[0]));
		}
	}

	return "";
}

void AutohostInterface::Send(boost::asio::mutable_buffers_1 buffer)
{
	if (autohost.is_open()) {
		try {
			autohost.send(buffer);
		} catch (boost::system::system_error& e) {
			autohost.close();
			LogObject() << "[AutohostInterface] Error: Failed to send buffer; the autohost may not be reachable: " << e.what();
		}
	}
}
