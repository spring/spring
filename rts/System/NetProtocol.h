#ifndef NETPROTOCOL_H
#define NETPROTOCOL_H

#include "BaseNetProtocol.h"

class CDemoRecorder;

/**
@brief Even higher level network code layer
@todo remove this since its mostly specific to other classes, not to network
The top of the networking stack.
*/
class CNetProtocol : public CBaseNetProtocol
{
public:
	CNetProtocol();
	~CNetProtocol();

	/**
	@brief Initialise in client mode (remote server)
	*/
	void InitClient(const char* server,unsigned portnum,unsigned sourceport, const unsigned wantedNumber);
	
	/**
	@brief Initialise in client mode (local server)
	 */
	void InitLocalClient(const unsigned wantedNumber);

	bool IsActiveConnection() const;

	bool localDemoPlayback;

	/**
	@brief Take a look at the messages that will be returned by GetData().
	@return A RawPacket holding the data, or 0 if no data
	@param ahead How many packets to look ahead. A typical usage would be:
	for (int ahead = 0; (packet = net->Peek(ahead)) != NULL; ++ahead) {}
	*/
	const RawPacket* Peek(unsigned ahead) const;

	/**
	@brief Recieve data from Client
	@return The amount of data recieved, or -1 if connection did not exists
	@todo Throw exceptions
	Recieves only one message (even if there are more in the recieve buffer), so call this until you get a 0 in return
	 */
	RawPacket* GetData();
	
	CDemoRecorder* GetDemoRecorder() const { return record; }
	
	volatile bool loading;
	void UpdateLoop();

private:
	unsigned serverSlot;
	CDemoRecorder* record;
};

extern CNetProtocol* net;

#endif
