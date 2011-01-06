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

#include "System/mmgr.h"

#include "ProtocolDef.h"
#include "UDPConnection.h"
#include "Socket.h"
#include "System/LogOutput.h"
#include "System/Platform/errorhandler.h"

namespace netcode
{
using namespace boost::asio;

UDPListener::UDPListener(int port, const std::string& ip)
{
	SocketPtr socket;

	if (UDPListener::TryBindSocket(port, &socket, ip)) {
		boost::asio::socket_base::non_blocking_io socketCommand(true);
		socket->io_control(socketCommand);

		mySocket = socket;
		acceptNewConnections = true;
	}

	if (!acceptNewConnections) {
		handleerror(NULL, "[UDPListener] error: unable to bind UDP port, see log for details.", "Network error", MBF_OK | MBF_EXCL);
	} else {
		LogObject() << "[UDPListener] succesfully bound socket on port " << port;
	}
}

bool UDPListener::TryBindSocket(int port, SocketPtr* socket, const std::string& ip) {

	std::string errorMsg = "";

	try {
		ip::address addr;
		boost::system::error_code err;

		socket->reset(new ip::udp::socket(netservice));
		(*socket)->open(ip::udp::v6(), err); // test IP v6 support

		const bool supportsIPv6 = !err;

		addr = WrapIP(ip, &err);
		if (ip.empty()) {
			// use the "any" address
			if (supportsIPv6) {
				addr = ip::address_v6::any();
			} else {
				addr = ip::address_v4::any();
			}
		} else if (err) {
			throw std::runtime_error("Failed to parse address " + ip + ": " + err.message());
		}

		if (!supportsIPv6 && addr.is_v6()) {
			throw std::runtime_error("IP v6 not supported, can not use address " + addr.to_string());
		}

		if (netcode::IsLoopbackAddress(addr)) {
			LogObject() << "WARNING: Opening socket on loopback address. Other users will not be able to connect!";
		}

		if (!addr.is_v6()) {
			if (supportsIPv6) {
				(*socket)->close();
			}
			(*socket)->open(ip::udp::v4(), err);
			if (err) {
				throw std::runtime_error("Failed to open IP V4 socket: " + err.message());
			}
		}

		LogObject() << "Binding UDP socket to IP " <<  (addr.is_v6() ? "(v6)" : "(v4)") << " " << addr << " Port " << port;
		(*socket)->bind(ip::udp::endpoint(addr, port));
	} catch (std::runtime_error& e) { // includes also boost::system::system_error, as it inherits from runtime_error
		socket->reset();
		errorMsg = e.what();
		if (errorMsg.empty()) {
			errorMsg = "Unknown problem";
		}
	}
	const bool isBound = errorMsg.empty();

	if (!isBound) {
		LogObject() << "Failed to bind UDP socket on IP " << ip << ", port " << port << ": " << errorMsg;
	}

	return isBound;
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

boost::shared_ptr<UDPConnection> UDPListener::SpawnConnection(const std::string& ip, const unsigned port)
{
	boost::shared_ptr<UDPConnection> temp(new UDPConnection(mySocket, ip::udp::endpoint(WrapIP(ip), port)));
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
