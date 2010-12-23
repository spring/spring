/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef _MSC_VER
#	include "StdAfx.h"
#elif defined(_WIN32)
#	include <windows.h>
#endif

#include "UDPListener.h"

#include <boost/weak_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <list>
#include <queue>

#include "mmgr.h"

#include "ProtocolDef.h"
#include "LogOutput.h"
#include "UDPConnection.h"
#include "Socket.h"
#include "Platform/errorhandler.h"

namespace netcode
{
using namespace boost::asio;

UDPListener::UDPListener(const std::string& ip, int port)
{
	SocketPtr socket;

	if (TryBindSocket(ip, port, &socket)) {
		boost::asio::socket_base::non_blocking_io socketCommand(true);
		socket->io_control(socketCommand);

		mySocket = socket;
		acceptNewConnections = true;
	}

	if (!acceptNewConnections) {
		handleerror(NULL, "[UDPListener] error: unable to bind UDP port.", "Network error", MBF_OK | MBF_EXCL);
	} else {
		LogObject() << "[UDPListener] succesfully bound socket on port " << port;
	}
}

bool UDPListener::TryBindSocket(const std::string& ip, int port, SocketPtr* socket) const {
	bool r = false;

	try {
		socket->reset(new ip::udp::socket(netservice));

		boost::system::error_code err;
		(*socket)->open(ip::udp::v6(), err); // test v6

		if (!err) {
			if (!ip.empty() && ip != "localhost") {
				(*socket)->bind(ip::udp::endpoint(ip::address_v6::from_string(ip), port));
			} else {
				(*socket)->bind(ip::udp::endpoint(ip::address_v6::any(), port));
			}
		} else {
			// fallback to v4
			(*socket)->open(ip::udp::v4());

			if (!ip.empty() && ip != "localhost") {
				(*socket)->bind(ip::udp::endpoint(ip::address_v4::from_string(ip), port));
			} else {
				(*socket)->bind(ip::udp::endpoint(ip::address_v4::any(), port));
			}
		}

		r = true;
	} catch (boost::system::system_error&) {
		socket->reset();
	}

	return r;
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
		boost::asio::ip::udp::socket::message_flags flags = 0;
		boost::system::error_code err;
		size_t bytesReceived = mySocket->receive_from(boost::asio::buffer(buffer), sender_endpoint, flags, err);
		if (CheckErrorCode(err))
			break;

		if (bytesReceived < Packet::headerSize)
			continue;

		Packet data(&buffer[0], bytesReceived);

		bool processed = false;
		for (std::list< boost::weak_ptr<UDPConnection> >::iterator i = conn.begin(); i != conn.end(); ++i)
		{
			boost::shared_ptr<UDPConnection> locked(*i);
			if (locked->CheckAddress(sender_endpoint))
			{
				locked->ProcessRawPacket(data);
				processed = true;
				break;
			}
		}
		
		if (!processed) // still have the packet (means no connection with the sender's address found)
		{
			if (acceptNewConnections && data.lastContinuous == -1 && data.nakType == 0)
			{
				if (!data.chunks.empty() && (*data.chunks.begin())->chunkNumber == 0)
				{
					// new client wants to connect
					boost::shared_ptr<UDPConnection> incoming(new UDPConnection(mySocket, sender_endpoint));
					waiting.push(incoming);
					conn.push_back(incoming);
					incoming->ProcessRawPacket(data);
				}
			}
			else
			{
				LogObject() << "Dropping packet from unknown IP: [" << sender_endpoint.address() << "]:" << sender_endpoint.port();
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
	boost::shared_ptr<UDPConnection> temp(new UDPConnection(mySocket, ip::udp::endpoint(WrapIP(address), port)));
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
