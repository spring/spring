/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UDPListener.h"

#ifdef DEBUG
	#include "System/SpringFormat.h"
#endif
#include "System/Misc/NonCopyable.h"

#include <memory>
#include <asio.hpp>
#include <cinttypes>
#include <queue>


#include "ProtocolDef.h"
#include "UDPConnection.h"
#include "Socket.h"
#include "System/Log/ILog.h"
#include "System/Platform/errorhandler.h"
#include "System/StringUtil.h" // for IntToString (header only)


namespace netcode
{
using namespace asio;

UDPListener::UDPListener(int port, const std::string& ip): acceptNewConnections(false)
{
	SocketPtr socket;

	const std::string err = TryBindSocket(port, &socket, ip);

	if (err.empty()) {
		socket->non_blocking(true);

		mySocket = socket;
		SetAcceptingConnections(true);
	}

	if (!IsAcceptingConnections())
		throw network_error(err);

	LOG("[%s] successfully bound socket on port %i", __func__, socket->local_endpoint().port());
}

UDPListener::~UDPListener() {
	for (const auto& p: dropMap) {
		LOG("[%s] dropped %lu packets from unknown IP %s", __func__, (unsigned long) p.second, (p.first).c_str());
	}
}


std::string UDPListener::TryBindSocket(int port, SocketPtr* socket, const std::string& ip) {

	std::string errorMsg = "";

	try {
		asio::error_code err;

		if ((port < 0) || (port > 65535))
			throw std::range_error("Port is out of range [0, 65535]: " + IntToString(port));

		socket->reset(new ip::udp::socket(netservice));
		(*socket)->open(ip::udp::v6(), err); // test IP v6 support

		const bool supportsIPv6 = !err;

		asio::ip::udp::endpoint endpoint = ResolveAddr(ip, port, &err);
		asio::ip::address address = endpoint.address();

		if (err)
			throw std::runtime_error("[UDPListener] failed to parse hostname \"" + ip + "\": " + err.message());

		// use the "any" address
		if (ip.empty())
			endpoint = ip::udp::endpoint(address = GetAnyAddress(supportsIPv6), port);

		if (!supportsIPv6 && address.is_v6())
			throw std::runtime_error("[UDPListener] IPv6 not supported, can not use address " + address.to_string());

		if (address.is_loopback())
			LOG_L(L_WARNING, "[UDPListener::%s] opening socket on loopback address, other users will not be able to connect!", __func__);

		if (address.is_v4()) {
			if (supportsIPv6)
				(*socket)->close();

			(*socket)->open(ip::udp::v4(), err);

			if (err)
				throw std::runtime_error("[UDPListener] failed to open IPv4 socket: " + err.message());
		}

		(*socket)->bind(endpoint);

		LOG(
			"[UDPListener::%s] binding UDP socket to IPv%d-address %s (%s) on port %i",
			__func__, (address.is_v6()? 6: 4), address.to_string().c_str(), ip.c_str(), endpoint.port()
		);
	} catch (const std::runtime_error& ex) { // includes asio::system_error and std::range_error
		socket->reset();
		errorMsg = ex.what();

		if (errorMsg.empty())
			errorMsg = "Unknown problem";

		LOG_L(L_ERROR, "[UDPListener::%s] binding UDP socket to IP %s failed: %s", __func__, ip.c_str(), errorMsg.c_str());
	}

	return errorMsg;
}

void UDPListener::Update() {
	netservice.poll();

	size_t bytes_avail = 0;

	while ((bytes_avail = mySocket->available()) > 0) {
		std::vector<std::uint8_t> buffer(bytes_avail);

		ip::udp::endpoint sender_endpoint;
		asio::ip::udp::socket::message_flags flags = 0;
		asio::error_code err;

		const size_t bytesReceived = mySocket->receive_from(asio::buffer(buffer), sender_endpoint, flags, err);

		const auto ci = connMap.find(sender_endpoint);
		const bool knownConnection = (ci != connMap.end());

		if (knownConnection && ci->second.expired())
			continue;

		if (CheckErrorCode(err))
			break;

		if (bytesReceived < Packet::headerSize)
			continue;

		Packet data(&buffer[0], bytesReceived);

		if (knownConnection) {
			ci->second.lock()->ProcessRawPacket(data);
		} else {
			// still have the packet (means no connection with the sender's address found)
			if (acceptNewConnections && data.lastContinuous == -1 && data.nakType == 0)	{
				if (!data.chunks.empty() && (*data.chunks.begin())->chunkNumber == 0) {
					// new client wants to connect
					std::shared_ptr<UDPConnection> incoming(new UDPConnection(mySocket, sender_endpoint));
					waiting.push(incoming);
					connMap[sender_endpoint] = incoming;
					incoming->ProcessRawPacket(data);
				}
			} else {
				const asio::ip::address& senderAddr = sender_endpoint.address();
				const std::string& senderIP = senderAddr.to_string();

				if (dropMap.find(senderIP) == dropMap.end()) {
					LOG_L(L_DEBUG, "[UDPListener::%s] dropping packet from unknown IP: [%s]:%i", __func__, senderIP.c_str(), sender_endpoint.port());
					dropMap[senderIP] = 0;
				} else {
					dropMap[senderIP] += 1;
				}

			#ifdef DEBUG
				std::string conns;
				for (auto it = connMap.cbegin(); it != connMap.cend(); ++it) {
					conns += spring::format(" [%s]:%i;", it->first.address().to_string().c_str(),it->first.port());
				}
				LOG_L(L_DEBUG, "[UDPListener::%s] open connections: %s", __func__, conns.c_str());
			#endif
			}
		}
	}

	for (auto i = connMap.cbegin(); i != connMap.cend(); ) {
		if (i->second.expired()) {
			LOG_L(L_DEBUG, "[UDPListener::%s] connection closed: [%s]:%i", __func__, i->first.address().to_string().c_str(), i->first.port());
			i = connMap.erase(i);
			continue;
		}
		i->second.lock()->Update();
		++i;
	}
}

std::shared_ptr<UDPConnection> UDPListener::SpawnConnection(const std::string& ip, const unsigned port)
{
	std::shared_ptr<UDPConnection> newConn(new UDPConnection(mySocket, ip::udp::endpoint(WrapIP(ip), port)));
	connMap[newConn->GetEndpoint()] = newConn;
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

std::weak_ptr<UDPConnection> UDPListener::PreviewConnection()
{
	return waiting.front();
}

std::shared_ptr<UDPConnection> UDPListener::AcceptConnection()
{
	std::shared_ptr<UDPConnection> newConn = waiting.front();
	waiting.pop();
	connMap[newConn->GetEndpoint()] = newConn;
	return newConn;
}

void UDPListener::RejectConnection()
{
	waiting.pop();
}

void UDPListener::UpdateConnections() {
	for (auto i = connMap.begin(); i != connMap.end(); ) {
		std::shared_ptr<UDPConnection> uc = i->second.lock();

		if (uc && i->first != uc->GetEndpoint()) {
			connMap[uc->GetEndpoint()] = uc; // inserting does not invalidate iterators
			i = connMap.erase(i);
			continue;
		}

		++i;
	}
}

}
