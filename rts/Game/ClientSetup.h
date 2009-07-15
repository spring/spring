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

	std::string hostip;
	int hostport;
	int sourceport; ///< the port clients will try to connect from
	int autohostport;

	bool isHost;
};

#endif // CLIENT_SETUP_H

