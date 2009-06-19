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
	GAME_LUAMSG = 20
};
}

AutohostInterface::AutohostInterface(int remoteport) : autohost(netcode::netservice)
{
	autohost.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6::loopback(), 0));
	autohost.connect(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6::loopback(), remoteport));
}

AutohostInterface::~AutohostInterface()
{
}

void AutohostInterface::SendStart()
{
	uchar msg = SERVER_STARTED;
	autohost.send(boost::asio::buffer(&msg, sizeof(uchar)));
}

void AutohostInterface::SendQuit()
{
	uchar msg = SERVER_QUIT;
	autohost.send(boost::asio::buffer(&msg, sizeof(uchar)));
}

void AutohostInterface::SendStartPlaying()
{
	uchar msg = SERVER_STARTPLAYING;
	autohost.send(boost::asio::buffer(&msg, sizeof(uchar)));
}

void AutohostInterface::SendGameOver()
{
	uchar msg = SERVER_GAMEOVER;
	autohost.send(boost::asio::buffer(&msg, sizeof(uchar)));
}

void AutohostInterface::SendPlayerJoined(uchar playerNum, const std::string& name)
{
	unsigned msgsize = 2*sizeof(uchar)+name.size();
	std::vector<boost::uint8_t> buffer(msgsize);
	buffer[0] = PLAYER_JOINED;
	buffer[1] = playerNum;
	strncpy((char*)(&buffer[2]), name.c_str(), name.size());
	autohost.send(boost::asio::buffer(buffer));
}

void AutohostInterface::SendPlayerLeft(uchar playerNum, uchar reason)
{
	uchar msg[3] = {PLAYER_LEFT, playerNum, reason};
	autohost.send(boost::asio::buffer(&msg, 3 * sizeof(uchar)));
}

void AutohostInterface::SendPlayerReady(uchar playerNum, uchar readyState)
{
	uchar msg[3] = {PLAYER_READY, playerNum, readyState};
	autohost.send(boost::asio::buffer(&msg, 3 * sizeof(uchar)));
}

void AutohostInterface::SendPlayerChat(uchar playerNum, uchar destination, const std::string& chatmsg)
{
	unsigned msgsize = 3*sizeof(uchar)+chatmsg.size();
	std::vector<boost::uint8_t> buffer(msgsize);
	buffer[0] = PLAYER_CHAT;
	buffer[1] = playerNum;
	buffer[2] = destination;
	strncpy((char*)(&buffer[3]), chatmsg.c_str(), chatmsg.size());
	autohost.send(boost::asio::buffer(buffer));
}

void AutohostInterface::SendPlayerDefeated(uchar playerNum)
{
	uchar msg[2] = {PLAYER_DEFEATED, playerNum};
	autohost.send(boost::asio::buffer(&msg, 2 * sizeof(uchar)));
}

void AutohostInterface::Message(const std::string& message)
{
	unsigned msgsize = sizeof(uchar) + message.size();
	std::vector<boost::uint8_t> buffer(msgsize);
	buffer[0] = SERVER_MESSAGE;
	strncpy((char*)(&buffer[1]), message.c_str(), message.size());
	autohost.send(boost::asio::buffer(buffer));
}

void AutohostInterface::Warning(const std::string& message)
{
	unsigned msgsize = sizeof(uchar) + message.size();
	std::vector<boost::uint8_t> buffer(msgsize);
	buffer[0] = SERVER_WARNING;
	strncpy((char*)(&buffer[1]), message.c_str(), message.size());
	autohost.send(boost::asio::buffer(buffer));
}

void AutohostInterface::SendLuaMsg(const boost::uint8_t* msg, size_t msgSize)
{
	std::vector<boost::uint8_t> buffer(msgSize+1);
	buffer[0] = GAME_LUAMSG;
	std::copy(msg, msg+msgSize, buffer.begin()+1);
	autohost.send(boost::asio::buffer(buffer));
}

std::string AutohostInterface::GetChatMessage()
{
	size_t bytes_avail = 0;
	if ((bytes_avail = autohost.available()) > 0)
	{
		std::vector<boost::uint8_t> buffer(bytes_avail+1, 0);
		size_t bytesReceived = autohost.receive(boost::asio::buffer(buffer));
		return std::string((char*)(&buffer[0]));
	}
	else
		return "";
}

