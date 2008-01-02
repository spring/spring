#include "UDPListener.h"

#include <boost/weak_ptr.hpp>

#include "ProtocolDef.h"

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

void UDPListener::Update(std::queue< boost::shared_ptr<CConnection> >& waitingQueue)
{
	for (std::list< boost::weak_ptr< UDPConnection> >::iterator i = conn.begin(); i != conn.end(); )
	{
		if (i->expired())
			i = conn.erase(i);
		else
			++i;
	}

	do
	{
		unsigned char buffer[4096];
		sockaddr_in fromAddr;
		unsigned recieved = mySocket->RecvFrom(buffer, 4096, &fromAddr);
		
		if (recieved < UDPConnection::hsize)
		{
			break;
		}
		
		RawPacket* data = new RawPacket(buffer, recieved);
		bool interrupt = false;
		for (std::list< boost::weak_ptr<UDPConnection> >::iterator i = conn.begin(); i != conn.end(); ++i)
		{
			boost::shared_ptr<UDPConnection> locked(*i);
			if (locked->CheckAddress(fromAddr))
			{
				locked->ProcessRawPacket(data);
				interrupt=true;
			}
		}
		
		if (interrupt)
			continue;
		
		const int packetNumber = *(int*)(data->data);
		const int lastInOrder = *(int*)(data->data+4);
		if (acceptNewConnections && packetNumber == 0 && lastInOrder == -1)
		{
			// new client wants to connect
			boost::shared_ptr<UDPConnection> incoming(new UDPConnection(mySocket, fromAddr));
			waitingQueue.push(incoming);
			conn.push_back(incoming);
			incoming->ProcessRawPacket(data);
		}
		else
		{
			// throw it
			delete data;
		}
		
	} while (true);
	
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

void UDPListener::SetWaitingForConnections(const bool state)
{
	acceptNewConnections = state;
}

bool UDPListener::GetWaitingForConnections() const
{
	return acceptNewConnections;
}

}
