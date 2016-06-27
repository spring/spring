/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UDPListener.h"

#ifdef DEBUG
	#include <boost/format.hpp>
#endif
#include <boost/weak_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/cstdint.hpp>
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

	const std::string err = TryBindSocket(port, &socket, ip);

	if (err.empty()) {
		boost::asio::socket_base::non_blocking_io socketCommand(true);
		socket->io_control(socketCommand);

		mySocket = socket;
		SetAcceptingConnections(true);
	}

	if (IsAcceptingConnections()) {
		LOG("[UDPListener] successfully bound socket on port %i", socket->local_endpoint().port());
	} else {
		throw network_error(err);
	}
}

std::string UDPListener::TryBindSocket(int port, SocketPtr* socket, const std::string& ip) {

	std::string errorMsg = "";

	try {
		boost::system::error_code err;

		if ((port < 0) || (port > 65535)) {
			throw std::range_error("Port is out of range [0, 65535]: " + IntToString(port));
		}

		socket->reset(new ip::udp::socket(netservice));
		(*socket)->open(ip::udp::v6(), err); // test IP v6 support

		const bool supportsIPv6 = !err;

		auto addr = ResolveAddr(ip, port, &err);
		if (ip.empty()) {
			// use the "any" address
			addr = ip::udp::endpoint(GetAnyAddress(supportsIPv6), port);
		} else if (err) {
			throw std::runtime_error("Failed to parse hostname \"" + ip + "\": " + err.message());
		}

		if (!supportsIPv6 && addr.address().is_v6()) {
			throw std::runtime_error("IP v6 not supported, can not use address " + addr.address().to_string());
		}

		if (addr.address().is_loopback()) {
			LOG_L(L_WARNING, "Opening socket on loopback address. Other users will not be able to connect!");
		}

		if (addr.address().is_v4()) {
			if (supportsIPv6) {
				(*socket)->close();
			}
			(*socket)->open(ip::udp::v4(), err);
			if (err) {
				throw std::runtime_error("Failed to open IP V4 socket: " + err.message());
			}
		}

		(*socket)->bind(addr);
		LOG("Binding UDP socket to IP %s %s (%s) port %i",
				(addr.address().is_v6() ? "(v6)" : "(v4)"), addr.address().to_string().c_str(), ip.c_str(),
				addr.port());
	} catch (const std::runtime_error& ex) { // includes boost::system::system_error and std::range_error
		socket->reset();
		errorMsg = ex.what();
		if (errorMsg.empty()) {
			errorMsg = "Unknown problem";
		}
		LOG_L(L_ERROR, "Binding UDP socket to IP %s failed: %s", ip.c_str(), errorMsg.c_str());
	}

	return errorMsg;
}

void UDPListener::Update() {
	netservice.poll();

	size_t bytes_avail = 0;

	while ((bytes_avail = mySocket->available()) > 0) {
		std::vector<boost::uint8_t> buffer(bytes_avail);
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
			i = conn.erase(i);
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
			i = conn.erase(i);
		}
		else
			++i;
	}
}

}
