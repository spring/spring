/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CLIENT_SETUP_H
#define CLIENT_SETUP_H

#include <string>


class ClientSetup
{
public:
	ClientSetup();

	void LoadFromStartScript(const std::string& setup);
	void SanityCheck();

	std::string myPlayerName;
	std::string myPasswd;
	std::string saveFile;
	std::string demoFile;

	//! if this client is not the server player, the IP address we connect to
	//! if this client is the server player, the IP address that other players connect to
	std::string hostIP;

	//! if this client is not the server player, the port which we connect over
	//! if this client is the server player, the port over which we accept incoming connections
	int hostPort;

	bool isHost;
};

#endif // CLIENT_SETUP_H
