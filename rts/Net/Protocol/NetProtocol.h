/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NET_PROTOCOL_H
#define NET_PROTOCOL_H

#include <atomic>
#include <string>
#include <memory>

#include "BaseNetProtocol.h" // not used in here, but in all files including this one
#include "System/Threading/SpringThreading.h"

class ClientSetup;
class CDemoRecorder;

namespace netcode
{
	class RawPacket;
	class CConnection;
}

/**
 * @brief Client interface for handling communication with the game server.
 *
 * Even when playing singleplayer, this is the way of communicating
 * with the server. It keeps the connection alive,
 * and is able to send and receive raw binary packets transparently
 * over the network.
*/
class CNetProtocol
{
public:
	CNetProtocol();
	~CNetProtocol();

	/**
	 * @brief Initialise in client mode (remote server)
	*/
	void InitClient(std::shared_ptr<ClientSetup> clientSetup, const std::string& clientVersion, const std::string& clientPlatform);

	/**
	 * @brief Initialise in client mode (local server)
	 */
	void InitLocalClient();

	/// Are we still connected (or did the connection time-out)?
	bool CheckTimeout(int nsecs = 0, bool initial = false) const;

	void AttemptReconnect(const std::string& myVersion, const std::string& myPlatform);

	bool NeedsReconnect();

	/// This checks if any data has been received yet
	bool Connected() const;

	std::string ConnectionStr() const;

	/**
	 * @brief Take a look at the messages in the recieve buffer (read-only)
	 * @return A RawPacket holding the data, or 0 if no data
	 * @param ahead How many packets to look ahead. A typical usage would be:
	 * for (int ahead = 0; (packet = clientNet->Peek(ahead)) != NULL; ++ahead) {}
	 */
	std::shared_ptr<const netcode::RawPacket> Peek(unsigned ahead) const;

	/**
	 * @brief Deletes a packet from the buffer
	 * @param index queue index number
	 * useful for messages that skips queuing and needs to be processed immediately
	 */
	void DeleteBufferPacketAt(unsigned index);

	float GetPacketTime(int frameNum) const;

	/**
	 * @brief Receive a single message (and remove it from the recieve buffer)
	 * @return The first data packet from the buffer, or 0 if there is no data
	 *
	 * Receives only one message at a time
	 * (even if there are more in the recieve buffer),
	 * so call this until you get a 0 in return.
	 * When a demo recorder is present it will be recorded.
	 */
	std::shared_ptr<const netcode::RawPacket> GetData(int framenum);

	/**
	 * @brief Send a message to the server
	 */
	void Send(std::shared_ptr<const netcode::RawPacket> pkt);
	/// @overload
	void Send(const netcode::RawPacket* pkt);

	/**
	 * Updates our network while the game loads to prevent timeouts.
	 * Runs until \a keepUpdating is false.
	 */
	void UpdateLoop();

	/// Must be called to send / recieve packets
	void Update();

	void Close(bool flush = false);

	void KeepUpdating(bool b) { keepUpdating = b; }

	void SetDemoRecorder(CDemoRecorder* r);
	CDemoRecorder* GetDemoRecorder() const;

	unsigned int GetNumWaitingServerPackets() const;
	unsigned int GetNumWaitingPingPackets() const;

private:
	std::atomic<bool> keepUpdating;

	spring::spinlock serverConnMutex;

	std::unique_ptr<netcode::CConnection> serverConn;
	std::unique_ptr<CDemoRecorder> demoRecorder;

	std::string userName;
	std::string userPasswd;
};

extern CNetProtocol* clientNet;

#endif // NET_PROTOCOL_H

