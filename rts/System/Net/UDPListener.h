/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _UDP_LISTENER_H
#define _UDP_LISTENER_H

#include "System/Misc/NonCopyable.h"
#include <memory>
#include <asio/ip/udp.hpp>
#include <map>
#include <queue>
#include <string>

namespace netcode
{
class UDPConnection;

/**
 * @brief Class for handling Connections on an UDPSocket
 * Use this class if you want to use a UDPSocket to connect to more than
 * one client.
 * You can Listen for new connections, initiate new ones and send/recieve data
 * to/from them.
 */
class UDPListener : spring::noncopyable
{
public:
	/**
	 * @brief Open a socket and make it ready for listening
	 * @param  port the port to bind the socket to
	 * @param  ip local IP to bind to, or "" for any
	 */
	UDPListener(int port, const std::string& ip = "");

	/**
	 * @brief close the socket and DELETE all connections
	 */
	~UDPListener();

	/**
	 * Try to bind a socket to a local address and port.
	 * If no IP or an empty one is given, this method will use
	 * the IP v6 any address "::", if IP v6 is supported.
	 * @param  port the port to bind the socket to
	 * @param  socket the socket to be bound
	 * @param  ip local IP (v4 or v6) to bind to,
	 *         the default value "" results in the v6 any address "::",
	 *         or the v4 equivalent "0.0.0.0", if v6 is no supported
	 */
	static std::string TryBindSocket(int port, std::shared_ptr<asio::ip::udp::socket>& sock, const std::string& ip = "");

	/**
	 * @brief Run this from time to time
	 * Recieve data from the socket and hand it to the associated UDPConnection,
	 * or open a new UDPConnection. It also Updates all of its connections.
	 */
	void Update();

	/**
	 * Set if we are accepting new connections
	 * or drop all data from unconnected addresses.
	 */
	void SetAcceptingConnections(const bool enable) { acceptNewConnections = enable; }
	bool IsAcceptingConnections() const { return acceptNewConnections; }
	bool HasIncomingConnections() const { return (!waiting.empty()); }

	/**
	 * @brief Initiate a connection
	 * Make a new connection to ip:port. It will be pushed back in conn.
	 */
	std::shared_ptr<UDPConnection> SpawnConnection(const std::string& ip, const unsigned port);

	std::weak_ptr<UDPConnection> PreviewConnection() { return (waiting.front()); }
	std::shared_ptr<UDPConnection> AcceptConnection();

	void RejectConnection() { waiting.pop(); }
	void UpdateConnections(); // Updates connections when the endpoint has been reconnected

private:
	/**
	 * @brief Do we accept packets from unknown sources?
	 * If true, we will create a new connection, if false, they get dropped.
	 */
	bool acceptNewConnections;

	/// socket being listened on
	std::shared_ptr<asio::ip::udp::socket> socket;

	std::vector<std::uint8_t> recvBuffer;

	/// all connections
	std::map< asio::ip::udp::endpoint, std::weak_ptr<UDPConnection> > connMap;
	std::map< std::string, size_t> dropMap;

	std::queue< std::shared_ptr<UDPConnection> > waiting;
};

}

#endif // _UDP_LISTENER_H
