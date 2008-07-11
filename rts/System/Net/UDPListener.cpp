#include "UDPListener.h"

#include <boost/weak_ptr.hpp>

#include "ProtocolDef.h"
#include "UDPConnection.h"
#include "UDPSocket.h"

namespace netcode
{

UDPListener::UDPListener(int port)
{
	boost::shared_ptr<UDPSocket> temp(new UDPSocket(port));
	mySocket = temp;
	acceptNewConnections = true;
}

UDPListener::~UDPListener()
{
}

void UDPListener::Update()
{
	for (std::list< boost::weak_ptr< UDPConnection> >::iterator i = conn.begin(); i != conn.end(); )
	{
		if (i->expired())
			i = conn.erase(i);
		else
			++i;
	}

	unsigned char buffer[4096];
	sockaddr_in fromAddr;
	unsigned recieved;
	while ((recieved = mySocket->RecvFrom(buffer, 4096, &fromAddr)) >= UDPConnection::hsize)
	{
		RawPacket* data = new RawPacket(buffer, recieved);

		for (std::list< boost::weak_ptr<UDPConnection> >::iterator i = conn.begin(); i != conn.end(); ++i)
		{
			boost::shared_ptr<UDPConnection> locked(*i);
			if (locked->CheckAddress(fromAddr))
			{
				locked->ProcessRawPacket(data);
				data = 0; // UDPConnection takes ownership of packet
				break;
			}
		}
		
		if (data) // still have the packet (means no connection with the sender's address found)
		{		
			const int packetNumber = *(int*)(data->data);
			const int lastInOrder = *(int*)(data->data+4);
			const unsigned char nak = data->data[8];
			if (acceptNewConnections && packetNumber == 0 && lastInOrder == -1 && nak == 0)
			{
				// new client wants to connect
				boost::shared_ptr<UDPConnection> incoming(new UDPConnection(mySocket, fromAddr));
				waiting.push(incoming);
				conn.push_back(incoming);
				incoming->ProcessRawPacket(data);
			}
			else
			{
				// throw it
				delete data;
			}
		}	
	}
	
	for (std::list< boost::weak_ptr< UDPConnection> >::iterator i = conn.begin(); i != conn.end(); ++i)
	{
		boost::shared_ptr<UDPConnection> temp = i->lock();
		temp->Update();
	}
}

boost::shared_ptr<UDPConnection> UDPListener::SpawnConnection(const std::string& address, const unsigned port)
{
	boost::shared_ptr<UDPConnection> temp(new UDPConnection(mySocket, address, port));
	conn.push_back(temp);
	return temp;
}

bool UDPListener::Listen(const bool state)
{
	acceptNewConnections = state;
	return acceptNewConnections;
}

bool UDPListener::Listen() const
{
	return acceptNewConnections;
}

bool UDPListener::HasIncomingConnections() const
{
	return !waiting.empty();
}

bool UDPListener::HasIncomingData(int timeout)
{
	return mySocket->HasIncomingData(timeout);
}

boost::weak_ptr<UDPConnection> UDPListener::PreviewConnection()
{
	return waiting.front();
}

boost::shared_ptr<UDPConnection> UDPListener::AcceptConnection()
{
	boost::shared_ptr<UDPConnection> newConn = waiting.front();
	waiting.pop();
	conn.push_back(newConn);
	return newConn;
}

void UDPListener::RejectConnection()
{
	waiting.pop();
}

}
