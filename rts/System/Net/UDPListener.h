#ifndef _UDPLISTENER 
#define _UDPLISTENER

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <list>

#include "UDPSocket.h"
#include "UDPConnection.h"

namespace netcode
{

/**
@brief Class for handling Connections on an UDPSocket
Use this class if you want to use a UDPSocket to connect to more than one other clients. You can Listen for new connections, initiate new ones and send/recieve data to/from them.
@author Karl-Robert Ernst
*/
class UDPListener : boost::noncopyable
{
public:
	/**
	@brief Open a socket and make it ready for listening
	*/
	UDPListener(int port, const ProtocolDef* const proto);
	
	/**
	@brief close the socket and DELETE all connections
	*/
	~UDPListener();
	
	/**
	@brief Run this from time to time
	This does: recieve data from the socket and hand it to the associated UDPConnection, or open a new UDPConnection. It also Updates all of its connections
	*/
	void Update();
	
	/**
	@brief Initiate a connection
	Make a new connection to address:port. It will be pushed back in conn
	*/
	boost::shared_ptr<UDPConnection> SpawnConnection(const std::string& address, const unsigned port);
	
	bool HasWaitingConnection() const;
	/**
	@brief Get the first waiting incoming Connection
	You can use this to recieve data from it without accepting. After this it can be Accept'ed or Reject'ed.
	@return a UDPConnection or 0 if there are no, DO NOT DELETE IT
	*/
	boost::shared_ptr<UDPConnection> GetWaitingConnection();
	
	/**
	@brief Set if we are going to accept new connections or drop all data from unconnected addresses
	*/
	void SetWaitingForConnections(const bool state);
	
	/**
	@brief Are we accepting new connections?
	*/
	bool GetWaitingForConnections() const;
	
private:	
	/**
	@brief Check which Connection has the specified Address
	IMPORTANT: also searches in waitingConn
	@return an iterator pointing at the connection, or at waitingConn.end() if nothing matches
	*/
	std::list< boost::weak_ptr< UDPConnection> >::iterator ResolveConnection(const sockaddr_in& addr) ;
	
	
	/**
	@brief Do we accept packets from unknown sources?
	If true, we will create a new connection, if false, it get dropped
	*/
	bool acceptNewConnections;
	
	/// Our socket
	boost::shared_ptr<UDPSocket> mySocket;
	
	/// all connections
	std::list< boost::weak_ptr< UDPConnection> > conn;
	std::list< boost::shared_ptr< UDPConnection> > waitingConn;
	
	const ProtocolDef* const proto;
};

}

#endif
