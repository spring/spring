#ifndef CLIENT_SETUP_H
#define CLIENT_SETUP_H

#include <string>

class ClientSetup
{
public:
	ClientSetup();

	void Init(const std::string& setup);

	int myPlayerNum;
	std::string myPlayerName;

	std::string hostip;
	int hostport;
	int sourceport; //the port clients will try to connect from
	int autohostport;

	bool isHost;
}; 

#endif