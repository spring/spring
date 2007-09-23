#include "UDPListener.h"

#include <boost/weak_ptr.hpp>
#include <iostream>

namespace netcode
{

UDPListener::UDPListener(int port, const ProtocolDef* myproto) : proto(myproto)
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
			if (i->expired())
			{
				i = conn.erase(i);
				--i;
			}
			else
			{
				boost::shared_ptr<UDPConnection> locked(*i);
				if (locked->CheckAddress(fromAddr))
				{
					locked->ProcessRawPacket(data);
					interrupt=true;
				}
			}
		}
		if (interrupt)
			continue;
	
		for (std::list< boost::shared_ptr< UDPConnection> >::iterator i = waitingConn.begin(); i != waitingConn.end(); ++i)
		{
			if ((*i)->CheckAddress(fromAddr))
			{
				(*i)->ProcessRawPacket(data);
				interrupt=true;
			}
		}
		if (interrupt)
			continue;
		
		if (acceptNewConnections)
		{
			// new client wants to connect
			boost::shared_ptr<UDPConnection> incoming(new UDPConnection(mySocket, fromAddr, proto));
			incoming->ProcessRawPacket(data);
			waitingConn.push_back(incoming);
		}
		else
		{
			// throw it
			delete data;
		}
		
	} while (true);
	
	for (std::list< boost::weak_ptr< UDPConnection> >::iterator i = conn.begin(); i != conn.end(); ++i)
	{
		if (i->expired())
		{
			i = conn.erase(i);
			--i;
		}
		else
		{
			boost::shared_ptr<UDPConnection> temp = i->lock();
			temp->Update();
		}
	}
}

boost::shared_ptr<UDPConnection> UDPListener::SpawnConnection(const std::string& address, const unsigned port)
{
	boost::shared_ptr<UDPConnection> temp(new UDPConnection(mySocket, address, port, proto));
	conn.push_back(temp);
	return temp;
}

bool UDPListener::HasWaitingConnection() const
{
	return !waitingConn.empty();
}

boost::shared_ptr<UDPConnection> UDPListener::GetWaitingConnection()
{
	if (HasWaitingConnection())
	{
		boost::shared_ptr<UDPConnection> temp = waitingConn.front();
		waitingConn.pop_front();
		conn.push_back(temp);
		return temp;
	}
	else
	{
		throw std::runtime_error("UDPListener has no waiting Connections");
	}
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
