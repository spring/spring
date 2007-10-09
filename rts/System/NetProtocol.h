#ifndef NETPROTOCOL_H
#define NETPROTOCOL_H

#include "BaseNetProtocol.h"

class CDemoRecorder;

/**
@brief Even higher level network code layer
@TODO remove this since its mostly specific to other classes, not to network
The top of the networking stack.
*/
class CNetProtocol : public CBaseNetProtocol {
public:
	typedef unsigned char uchar;
	typedef unsigned int uint;

	CNetProtocol();
	~CNetProtocol();

	/**
	@brief Initialise in servermode
	@param portnum Portnumber to use
	@throw network_error If phailed (port already used or invalid)
	@return always 0
	*/
	int InitServer(const unsigned portnum);
	
	/**
	@brief Initialise in client mode (remote server)
	*/
	unsigned InitClient(const char* server,unsigned portnum,unsigned sourceport, const unsigned wantedNumber);
	
	/**
	@brief Initialise in client mode (local server)
	 */
	unsigned InitLocalClient(const unsigned wantedNumber);
	
	/**
	@brief Connect a local client
	@todo Merge with incoming connection handling for UDPConnections
	 */
	unsigned ServerInitLocalClient(const unsigned wantedNumber);

	/**
	@brief Update all internals
	1. Updates our CNet
	2. Check for incoming connections, accept them when they are valid and send some information
	*/
	void Update();

	bool IsDemoServer() const;
	bool localDemoPlayback;

	/**
	@brief Recieve data from Client
	@return The amount of data recieved, or -1 if connection did not exists
	@todo Throw exceptions
	Recieves only one message (even if there are more in the recieve buffer), so call this until you get a 0 in return
	 */
	int GetData(unsigned char* buf, const unsigned conNum);
	
	void SendScript(const std::string& scriptName);
	void SendMapName(const uint checksum, const std::string& mapName);
	void SendModName(const uint checksum, const std::string& modName);

	CDemoRecorder* GetDemoRecorder() const { return record; }

private:
	CDemoRecorder* record;

	// Game settings (only important for the server)
	//TODO move to CGameServer
	std::string scriptName;
	uint mapChecksum;
	std::string mapName;
	uint modChecksum;
	std::string modName;
};

extern CNetProtocol* net;

#endif
