/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LOCAL_CONNECTION_H
#define _LOCAL_CONNECTION_H

#include <deque>
#include "System/Threading/SpringMutex.h"

#include "Connection.h"

namespace netcode {

/**
 * @brief Class for local connection between server / client
 * Directly connects the respective input-buffers, to increase performance.
 * The server and the client have to run in one instance (same process)
 * of spring for this to work.
 * Otherwise, a normal UDP connection had to be used.
 * IMPORTANT: You must not have more than two instances of this.
 */
class CLocalConnection : public CConnection
{
public:
	/**
	 * @brief Constructor
	 * @throw network_error When there already 2 instances
	 */
	CLocalConnection();
	virtual ~CLocalConnection();

	// START overriding CConnection

	void SendData(boost::shared_ptr<const RawPacket> packet);
	bool HasIncomingData() const;
	boost::shared_ptr<const RawPacket> Peek(unsigned ahead) const;
	boost::shared_ptr<const RawPacket> GetData();
	void DeleteBufferPacketAt(unsigned index);
	void Flush(const bool forced) {}
	bool CheckTimeout(int seconds, bool initial) const { return false; }

	void ReconnectTo(CConnection& conn) {}
	bool CanReconnect() const { return false; }
	bool NeedsReconnect() { return false; }
	void Unmute() {}
	void Close(bool flush);
	void SetLossFactor(int factor) {}

	unsigned int GetPacketQueueSize() const;

	std::string Statistics() const;
	std::string GetFullAddress() const;

	// END overriding CConnection

private:
	static std::deque< boost::shared_ptr<const RawPacket> > pqueues[2];
	static spring::mutex mutexes[2];

	unsigned int OtherInstance() const { return ((instance + 1) % 2); }

	/// we can have two instances, one in GameServer and one in NetProtocol
	/// (first instance represents server->client and second client->server)
	static unsigned int instances;
	/// which instance we are
	unsigned int instance;
};

} // namespace netcode

#endif // _LOCAL_CONNECTION_H

