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
@enum EVENT Which events can be sent to the autohost (in bracets: parameters, where uchar means unsigned char and "string" means plain ascii text)
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
 
	/// Player has left (uchar playernumber, uchar reason (0: lost connection, 1: left, 2: kicked) )
	PLAYER_LEFT = 11,
 
	/// Player has updated its ready-state (uchar playernumber, uchar state (0: not ready, 1: ready, 2: state not changed) )
	PLAYER_READY = 12,

	/**
	@brief Player has sent a chat message (uchar playernumber, uchar destination, string text)
	Destination can be any of: a playernumber [0-32]
	static const int TO_ALLIES = 127;
	static const int TO_SPECTATORS = 126;
	static const int TO_EVERYONE = 125;
	(copied from Game/ChatMessage.h)
	*/
	PLAYER_CHAT = 13,
 
	/// Player has been defeated (uchar playernumber)
	PLAYER_DEFEATED = 14,

	/**
	 * @brief Message sent by lua script
	 * 
	 * (uchar playernumber, uint16_t script, uint8_t mode, uint8_t[X] data) (X = space left in packet)
	 * */
	GAME_LUAMSG = 20,
	
	/// team statistics, see CTeam::Statistics for reference how to read them
	/**
	* (uchar teamnumber), CTeam::Statistics(in binary form)
	*/
	GAME_TEAMSTAT = 60
};
}

using namespace boost::asio;
AutohostInterface::AutohostInterface(const std::string& autohostip, int remoteport) : autohost(netcode::netservice)
{
	boost::system::error_code err;
	autohost.open(ip::udp::v6(), err); // test v6

	if (!err) {
		autohost.bind(ip::udp::endpoint(ip::address_v6::any(), 0));
	} else {
		LogObject() << "[AutohostInterface] IPv6 not supported, falling back to v4";
		autohost.open(ip::udp::v4());
		autohost.bind(ip::udp::endpoint(ip::address_v4::any(), 0));
	}

	boost::asio::socket_base::non_blocking_io command(true);
	autohost.io_control(command);

	std::string connectErrorMsg;

	try {
		boost::system::error_code connectError;
		autohost.connect(netcode::ResolveAddr(autohostip, remoteport), connectError);

		if (connectError) {
			connectErrorMsg = connectError.message();
			autohost.close();
		}
	} catch (boost::system::system_error& e) {
		connectErrorMsg = e.what();
		autohost.close();
	}

	if (!autohost.is_open()) {
		LogObject() << "could not open autohost socket: " << connectErrorMsg;
	}
}

AutohostInterface::~AutohostInterface()
{
}

void AutohostInterface::SendStart()
{
	uchar msg = SERVER_STARTED;

	if (autohost.is_open()) {
		autohost.send(boost::asio::buffer(&msg, sizeof(uchar)));
	}
}

void AutohostInterface::SendQuit()
{
	uchar msg = SERVER_QUIT;

	if (autohost.is_open()) {
		autohost.send(boost::asio::buffer(&msg, sizeof(uchar)));
	}
}

void AutohostInterface::SendStartPlaying()
{
	uchar msg = SERVER_STARTPLAYING;

	if (autohost.is_open()) {
		autohost.send(boost::asio::buffer(&msg, sizeof(uchar)));
	}
}

void AutohostInterface::SendGameOver()
{
	uchar msg = SERVER_GAMEOVER;

	if (autohost.is_open()) {
		autohost.send(boost::asio::buffer(&msg, sizeof(uchar)));
	}
}

void AutohostInterface::SendPlayerJoined(uchar playerNum, const std::string& name)
{
	if (autohost.is_open()) {
		unsigned msgsize = 2 * sizeof(uchar) + name.size();
		std::vector<boost::uint8_t> buffer(msgsize);
		buffer[0] = PLAYER_JOINED;
		buffer[1] = playerNum;
		strncpy((char*)(&buffer[2]), name.c_str(), name.size());

		autohost.send(boost::asio::buffer(buffer));
	}
}

void AutohostInterface::SendPlayerLeft(uchar playerNum, uchar reason)
{
	uchar msg[3] = {PLAYER_LEFT, playerNum, reason};

	if (autohost.is_open()) {
		autohost.send(boost::asio::buffer(&msg, 3 * sizeof(uchar)));
	}
}

void AutohostInterface::SendPlayerReady(uchar playerNum, uchar readyState)
{
	uchar msg[3] = {PLAYER_READY, playerNum, readyState};

	if (autohost.is_open()) {
		autohost.send(boost::asio::buffer(&msg, 3 * sizeof(uchar)));
	}
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

		autohost.send(boost::asio::buffer(buffer));
	}
}

void AutohostInterface::SendPlayerDefeated(uchar playerNum)
{
	uchar msg[2] = {PLAYER_DEFEATED, playerNum};

	if (autohost.is_open()) {
		autohost.send(boost::asio::buffer(&msg, 2 * sizeof(uchar)));
	}
}

void AutohostInterface::Message(const std::string& message)
{
	if (autohost.is_open()) {
		const unsigned msgsize = sizeof(uchar) + message.size();
		std::vector<boost::uint8_t> buffer(msgsize);
		buffer[0] = SERVER_MESSAGE;
		strncpy((char*)(&buffer[1]), message.c_str(), message.size());

		autohost.send(boost::asio::buffer(buffer));
	}
}

void AutohostInterface::Warning(const std::string& message)
{
	if (autohost.is_open()) {
		const unsigned msgsize = sizeof(uchar) + message.size();
		std::vector<boost::uint8_t> buffer(msgsize);
		buffer[0] = SERVER_WARNING;
		strncpy((char*)(&buffer[1]), message.c_str(), message.size());

		autohost.send(boost::asio::buffer(buffer));
	}
}

void AutohostInterface::SendLuaMsg(const boost::uint8_t* msg, size_t msgSize)
{
	if (autohost.is_open()) {
		std::vector<boost::uint8_t> buffer(msgSize+1);
		buffer[0] = GAME_LUAMSG;
		std::copy(msg, msg + msgSize, buffer.begin() + 1);

		autohost.send(boost::asio::buffer(buffer));
	}
}

void AutohostInterface::Send(const boost::uint8_t* msg, size_t msgSize)
{
	if (autohost.is_open()) {
		std::vector<boost::uint8_t> buffer(msgSize);
		std::copy(msg, msg + msgSize, buffer.begin());

		autohost.send(boost::asio::buffer(buffer));
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
