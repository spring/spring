/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LOCAL_CONNECTION_H
#define _LOCAL_CONNECTION_H

#include <deque>
#include "System/Threading/SpringThreading.h"

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
	~CLocalConnection();

	// START overriding CConnection

	void SendData(std::shared_ptr<const RawPacket> pkt) override;
	bool HasIncomingData() const override;
	std::shared_ptr<const RawPacket> Peek(unsigned ahead) const override;
	std::shared_ptr<const RawPacket> GetData() override;
	void DeleteBufferPacketAt(unsigned index) override;
	void Flush(const bool forced) override {}
	bool CheckTimeout(int seconds, bool initial) const override { return false; }

	void ReconnectTo(CConnection& conn) override {}
	bool CanReconnect() const override { return false; }
	bool NeedsReconnect() override { return false; }
	void Unmute() override {}
	void Close(bool flush) override;
	void SetLossFactor(int factor) override {}

	unsigned int GetPacketQueueSize() const override;

	std::string Statistics() const override;
	std::string GetFullAddress() const override { return "Localhost"; }

	// END overriding CConnection

private:
	static constexpr unsigned int MAX_INSTANCES = 2;

	static std::deque< std::shared_ptr<const RawPacket> > pktQueues[MAX_INSTANCES];
	static spring::mutex mutexes[MAX_INSTANCES];
	static CLocalConnection* instancePtrs[MAX_INSTANCES];

	unsigned int RemoteInstanceIdx() const { return ((instanceIdx + 1) % MAX_INSTANCES); }

	/// we can have two instances, one in GameServer and one in NetProtocol
	/// (first instance represents server->client and second client->server)
	static unsigned int numInstances;
	/// which instance we are
	unsigned int instanceIdx;
};

} // namespace netcode

#endif // _LOCAL_CONNECTION_H

