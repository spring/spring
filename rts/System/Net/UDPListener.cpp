#include "UDPListener.h"

namespace netcode
{

UDPListener::UDPListener(int port)
{
	mySocket = new UDPSocket(port);
	acceptNewConnections = true;
}

UDPListener::~UDPListener()
{
	
	for (conn_list::iterator i = conn.begin(); i != conn.end(); ++i)
	{
		delete *i;
		*i = 0;
	}
	
	for (conn_list::iterator i = waitingConn.begin(); i != waitingConn.end(); ++i)
	{
		delete *i;
		*i = 0;
	}
	
	delete mySocket;
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
		conn_list::iterator connPos = ResolveConnection(fromAddr);
		if (connPos != waitingConn.end())
		{
			(*connPos)->ProcessRawPacket(data);
		}
		else if (acceptNewConnections)
		{
			// new client wants to connect
			waitingConn.push_back(new UDPConnection(mySocket, fromAddr));
			waitingConn.back()->ProcessRawPacket(data);
		}
		else
		{
			// throw it
			delete data;
		}
		
	} while (true);
	
	for (conn_list::iterator i = conn.begin(); i != conn.end(); ++i)
	{
		(*i)->Update();
	}
}

UDPConnection* UDPListener::SpawnConnection(const std::string& address, const unsigned port)
{
	conn.push_back(new UDPConnection(mySocket, address, port));
	return conn.back();
}

UDPConnection* UDPListener::GetWaitingConnection() const
{
	if (!waitingConn.empty())
	{
		return waitingConn.front();
	}
	else
	{
		return 0;
	}
}

void UDPListener::AcceptWaitingConnection()
{
	conn.push_back(waitingConn.front());
	waitingConn.pop_front();
}

void UDPListener::RejectWaitingConnection()
{
	delete waitingConn.front();
	waitingConn.pop_front();
}

void UDPListener::SetWaitingForConnections(const bool state)
{
	acceptNewConnections = state;
}

bool UDPListener::GetWaitingForConnections() const
{
	return acceptNewConnections;
}

std::list<UDPConnection*>::iterator UDPListener::ResolveConnection(const sockaddr_in& addr)
{
	for (conn_list::iterator i = conn.begin(); i != conn.end(); ++i)
	{
		if ((*i)->CheckAddress(addr))
			return i;
	}
	
	for (conn_list::iterator i = waitingConn.begin(); i != waitingConn.end(); ++i)
	{
		if ((*i)->CheckAddress(addr))
			return i;
	}
	
	return waitingConn.end();
}

}
