#include "AutohostInterface.h"

#include <string.h>
#include "Net/UDPConnectedSocket.h" 

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
 
	/// Player has sent a chat message (uchar playernumber, string text)
	PLAYER_CHAT = 13,
 
	/// Player has been defeated (uchar playernumber)
	PLAYER_DEFEATED = 14
};
}

AutohostInterface::AutohostInterface(int remoteport)
{
	autohost = new netcode::UDPConnectedSocket("127.0.0.1", remoteport);
}

AutohostInterface::~AutohostInterface()
{
	delete autohost;
}

void AutohostInterface::SendStart() const
{
	uchar msg = SERVER_STARTED;
	autohost->Send(&msg, sizeof(uchar));
}

void AutohostInterface::SendQuit() const
{
	uchar msg = SERVER_QUIT;
	autohost->Send(&msg, sizeof(uchar));
}

void AutohostInterface::SendStartPlaying() const
{
	uchar msg = SERVER_STARTPLAYING;
	autohost->Send(&msg, sizeof(uchar));
}

void AutohostInterface::SendGameOver() const
{
	uchar msg = SERVER_GAMEOVER;
	autohost->Send(&msg, sizeof(uchar));
}

void AutohostInterface::SendPlayerJoined(uchar playerNum, const std::string& name) const
{
	unsigned msgsize = 2*sizeof(uchar)+name.size();
	uchar* msg = new uchar[msgsize];
	msg[0] = PLAYER_JOINED;
	msg[1] = playerNum;
	strncpy((char*)msg+2, name.c_str(), name.size());
	autohost->Send(msg, msgsize);
	delete[] msg;
}

void AutohostInterface::SendPlayerLeft(uchar playerNum, uchar reason) const
{
	uchar msg[3] = {PLAYER_LEFT, playerNum, reason};
	autohost->Send(msg, 3);
}

void AutohostInterface::SendPlayerReady(uchar playerNum, uchar readyState) const
{
	uchar msg[3] = {PLAYER_READY, playerNum, readyState};
	autohost->Send(msg, 3);
}

void AutohostInterface::SendPlayerChat(uchar playerNum, const std::string& chatmsg) const
{
	unsigned msgsize = 2*sizeof(uchar)+chatmsg.size();
	uchar* msg = new uchar[msgsize];
	msg[0] = PLAYER_CHAT;
	msg[1] = playerNum;
	strncpy((char*)msg+2, chatmsg.c_str(), chatmsg.size());
	autohost->Send(msg, msgsize);
	delete[] msg;
}

void AutohostInterface::SendPlayerDefeated(uchar playerNum) const
{
	uchar msg[2] = {PLAYER_READY, playerNum};
	autohost->Send(msg, 2);
}

void AutohostInterface::Message(const std::string& message)
{
	unsigned msgsize = sizeof(uchar) + message.size();
	uchar* msg = new uchar[msgsize];
	msg[0] = SERVER_MESSAGE;
	strncpy((char*)msg+1, message.c_str(), message.size());
	autohost->Send(msg, msgsize);
	delete[] msg;
}

void AutohostInterface::Warning(const std::string& message)
{
	unsigned msgsize = sizeof(uchar) + message.size();
	uchar* msg = new uchar[msgsize];
	msg[0] = SERVER_WARNING;
	strncpy((char*)msg+1, message.c_str(), message.size());
	autohost->Send(msg, msgsize);
	delete[] msg;
}

std::string AutohostInterface::GetChatMessage() const
{
	uchar buffer[4096];
	unsigned length = autohost->Recv(buffer, 4096);
	buffer[length] = '\0';
	
	std::string msg((char*)buffer);
	return msg;
}

