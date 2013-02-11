/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UDPListener.h"

#if defined(_WIN32)
#	include <windows.h>
#endif

#ifdef DEBUG
	#include <boost/format.hpp>
#endif
#include <boost/weak_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <list>
#include <queue>


#include "ProtocolDef.h"
#include "UDPConnection.h"
#include "Socket.h"
#include "System/Log/ILog.h"
#include "System/Platform/errorhandler.h"
#include "System/Util.h" // for IntToString (header only)

namespace netcode
{
using namespace boost::asio;

UDPListener::UDPListener(int port, const std::string& ip)
	: acceptNewConnections(false)
{
	SocketPtr socket;

	if (UDPListener::TryBindSocket(port, &socket, ip)) {
		boost::asio::socket_base::non_blocking_io socketCommand(true);
		socket->io_control(socketCommand);

		mySocket = socket;
		SetAcceptingConnections(true);
	}

	if (IsAcceptingConnections()) {
		LOG("[UDPListener] successfully bound socket on port %i", port);
	} else {
		handleerror(NULL, "[UDPListener] error: unable to bind UDP port, see log for details.", "Network error", MBF_OK | MBF_EXCL);
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
			LOG_L(L_WARNING, "Opening socket on loopback address. Other users will not be able to connect!");
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

		if ((port < 0) || (port > 65535)) {
			throw std::range_error("Port is out of range [0, 65535]: " + IntToString(port));
		}

		LOG("Binding UDP socket to IP %s %s port %i",
				(addr.is_v6() ? "(v6)" : "(v4)"), addr.to_string().c_str(),
				port);
		(*socket)->bind(ip::udp::endpoint(addr, port));
	} catch (const std::runtime_error& ex) { // includes boost::system::system_error and std::range_error
		socket->reset();
		errorMsg = ex.what();
		if (errorMsg.empty()) {
			errorMsg = "Unknown problem";
		}
	}
	const bool isBound = errorMsg.empty();

	if (!isBound) {
		LOG_L(L_ERROR, "Failed to bind UDP socket on IP %s, port %i: %s",
				ip.c_str(), port, errorMsg.c_str());
	}

	return isBound;
}

void UDPListener::Update() {
	netservice.poll();

	size_t bytes_avail = 0;

	while ((bytes_avail = mySocket->available()) > 0) {
		std::vector<uint8_t> buffer(bytes_avail);
		ip::udp::endpoint sender_endpoint;
		boost::asio::ip::udp::socket::message_flags flags = 0;
		boost::system::error_code err;
		size_t bytesReceived = mySocket->receive_from(boost::asio::buffer(buffer), sender_endpoint, flags, err);

		ConnMap::iterator ci = conn.find(sender_endpoint);
		bool knownConnection = (ci != conn.end());

		if (knownConnection && ci->second.expired())
			continue;

		if (CheckErrorCode(err))
			break;

		if (bytesReceived < Packet::headerSize)
			continue;

		Packet data(&buffer[0], bytesReceived);

		if (knownConnection) {
			ci->second.lock()->ProcessRawPacket(data);
		}
		else { // still have the packet (means no connection with the sender's address found)
			if (acceptNewConnections && data.lastContinuous == -1 && data.nakType == 0)	{
				if (!data.chunks.empty() && (*data.chunks.begin())->chunkNumber == 0) {
					// new client wants to connect
					boost::shared_ptr<UDPConnection> incoming(new UDPConnection(mySocket, sender_endpoint));
					waiting.push(incoming);
					conn[sender_endpoint] = incoming;
					incoming->ProcessRawPacket(data);
				}
			}
			else {
				LOG_L(L_WARNING, "Dropping packet from unknown IP: [%s]:%i",
						sender_endpoint.address().to_string().c_str(),
						sender_endpoint.port());
			#ifdef DEBUG
				std::string conns;
				for (ConnMap::iterator it = conn.begin(); it != conn.end(); ++it) {
					conns += str(boost::format(" [%s]:%i;") %it->first.address().to_string().c_str() %it->first.port());
				}
				LOG_L(L_DEBUG, "Open connections: %s", conns.c_str());
			#endif
			}
		}
	}

	for (ConnMap::iterator i = conn.begin(); i != conn.end(); ) {
		if (i->second.expired()) {
			LOG_L(L_DEBUG, "Connection closed: [%s]:%i", i->first.address().to_string().c_str(), i->first.port());
			i = set_erase(conn, i);
			continue;
		}
		i->second.lock()->Update();
		++i;
	}
}

boost::shared_ptr<UDPConnection> UDPListener::SpawnConnection(const std::string& ip, const unsigned port)
{
	boost::shared_ptr<UDPConnection> newConn(new UDPConnection(mySocket, ip::udp::endpoint(WrapIP(ip), port)));
	conn[newConn->GetEndpoint()] = newConn;
	return newConn;
}

void UDPListener::SetAcceptingConnections(const bool enable)
{
	acceptNewConnections = enable;
}

bool UDPListener::IsAcceptingConnections() const
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
	conn[newConn->GetEndpoint()] = newConn;
	return newConn;
}

void UDPListener::RejectConnection()
{
	waiting.pop();
}

void UDPListener::UpdateConnections() {
	for (ConnMap::iterator i = conn.begin(); i != conn.end(); ) {
		boost::shared_ptr<UDPConnection> uc = i->second.lock();
		if (uc && i->first != uc->GetEndpoint()) {
			conn[uc->GetEndpoint()] = uc; // inserting does not invalidate iterators
			i = set_erase(conn, i);
		}
		else
			++i;
	}
}

}
