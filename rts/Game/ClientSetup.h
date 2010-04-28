/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CLIENT_SETUP_H
#define CLIENT_SETUP_H

#include <string>

class ClientSetup
{
public:
	static const unsigned int DEFAULT_HOST_PORT = 8452;

	ClientSetup();

	void Init(const std::string& setup);

	std::string myPlayerName;
	std::string myPasswd;

	std::string hostip;
	int hostport;

	bool isHost;
};

#endif // CLIENT_SETUP_H

