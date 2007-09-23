#ifndef _UDPHARDLINK
#define _UDPHARDLINK

#include "UDPSocket.h"

namespace netcode {

/**
@brief Simple Class for communication with a socket location
This is simply a UDPSocket which has been connect()'ed to a specific address and will send / recieve only to / from this address
@author Karl-Robert Ernst
*/
class UDPHardlink : private UDPSocket
{
public:
	/**
	@brief Constructor
	@param address The DNS or IP of the other side
	@param remoteport the port the other side use
	@param port The port we will use here
	@param range if $port is blocked, try port +1, +2, ... +range
	@throw network_error when hostname cannot be resolved
	*/
	UDPHardlink(const char* const address, const unsigned remoteport, const int port, const unsigned range=0);

	/**
	@brief Send some data
	*/
	void Send(const unsigned char* const buf, const unsigned dataLength) const;
	
	/**
	@brief Recieve some data
	*/
	unsigned Recv(unsigned char* buf, const unsigned bufLength) const;

private:

};



} // namespace netcode

#endif


