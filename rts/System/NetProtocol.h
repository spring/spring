#ifndef NETPROTOCOL_H
#define NETPROTOCOL_H

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "BaseNetProtocol.h"

class CDemoRecorder;
namespace netcode
{
	class RawPacket;
	class CConnection;
};

/**
@brief Even higher level network code layer

The top of the networking stack.
*/
class CNetProtocol
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

	bool Active() const;
	bool Connected() const;

	bool localDemoPlayback;

	/**
	@brief Take a look at the messages that will be returned by GetData().
	@return A RawPacket holding the data, or 0 if no data
	@param ahead How many packets to look ahead. A typical usage would be:
	for (int ahead = 0; (packet = net->Peek(ahead)) != NULL; ++ahead) {}
	*/
	boost::shared_ptr<const netcode::RawPacket> Peek(unsigned ahead) const;

	/**
	@brief Receive data from Client
	@return The data packet, or 0 if there is no data
	@throw network_error If there is no such connection
	
	Receives only one message (even if there are more in the recieve buffer), so call this until you get a 0 in return
	 */
	boost::shared_ptr<const netcode::RawPacket> GetData();
	
	void Send(boost::shared_ptr<const netcode::RawPacket> pkt);
	void Send(const netcode::RawPacket* pkt);
	
	CDemoRecorder* GetDemoRecorder() const { return record.get(); }

	/// updates our network while the game loads to prevent timeouts
	void UpdateLoop();
	void Update();
	volatile bool loading;

private:
	bool isLocal;
	boost::scoped_ptr<netcode::CConnection> server;
	boost::scoped_ptr<CDemoRecorder> record;
};

extern CNetProtocol* net;

#endif
