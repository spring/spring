#ifdef _MSC_VER
#	include "StdAfx.h"
#elif defined(_WIN32)
#	include <windows.h>
#endif

#include "UDPListener.h"

#ifndef _MSC_VER
#include "StdAfx.h"
#endif

#include <boost/weak_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <list>
#include <queue>

#include "mmgr.h"

#include "ProtocolDef.h"
#include "UDPConnection.h"
#include "Socket.h"
#include "LogOutput.h"

namespace netcode
{
using namespace boost::asio;

UDPListener::UDPListener(int port)
{
	SocketPtr temp(new ip::udp::socket(netservice));

	boost::system::error_code err;
	temp->open(ip::udp::v6(), err); // test v6
	if (!err)
	{
		temp->bind(ip::udp::endpoint(ip::address_v6::any(), port));
	}
	else
	{
		// fallback to v4
		temp->open(ip::udp::v4());
		temp->bind(ip::udp::endpoint(ip::address_v4::any(), port));
	}
	boost::asio::socket_base::non_blocking_io command(true);
	temp->io_control(command);

	mySocket = temp;
	acceptNewConnections = true;
}

UDPListener::~UDPListener()
{
}

void UDPListener::Update()
{
	netservice.poll();
	for (std::list< boost::weak_ptr< UDPConnection> >::iterator i = conn.begin(); i != conn.end(); )
	{
		if (i->expired())
			i = conn.erase(i);
		else
			++i;
	}

	size_t bytes_avail = 0;

	while ((bytes_avail = mySocket->available()) > 0)
	{
		std::vector<uint8_t> buffer(bytes_avail);
		ip::udp::endpoint sender_endpoint;
		size_t bytesReceived = mySocket->receive_from(boost::asio::buffer(buffer), sender_endpoint);

		if (bytesReceived < UDPConnection::hsize)
			continue;

		RawPacket* data = new RawPacket(&buffer[0], bytesReceived);

		for (std::list< boost::weak_ptr<UDPConnection> >::iterator i = conn.begin(); i != conn.end(); ++i)
		{
			boost::shared_ptr<UDPConnection> locked(*i);
			if (locked->CheckAddress(sender_endpoint))
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
				boost::shared_ptr<UDPConnection> incoming(new UDPConnection(mySocket, sender_endpoint));
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
	boost::shared_ptr<UDPConnection> temp(new UDPConnection(mySocket, ip::udp::endpoint(ip::address_v4::from_string(address), port)));
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
