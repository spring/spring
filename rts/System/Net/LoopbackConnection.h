/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LOOPBACK_CONNECTION_H
#define _LOOPBACK_CONNECTION_H

#include <deque>

#include "Connection.h"

namespace netcode {

/**
 * @brief A dummy queue-like connection that bounces everything back to the sender
 */
class CLoopbackConnection : public CConnection
{
public:
	void SendData(std::shared_ptr<const RawPacket> pkt) override;
	bool HasIncomingData() const override { return (!pktQueue.empty()); }
	std::shared_ptr<const RawPacket> Peek(unsigned ahead) const override;
	std::shared_ptr<const RawPacket> GetData() override;
	void DeleteBufferPacketAt(unsigned index) override;
	void Flush(const bool forced) override {}
	bool CheckTimeout(int seconds, bool initial) const override { return false; }

	void ReconnectTo(CConnection& conn) override {}
	bool CanReconnect() const override { return false; }
	bool NeedsReconnect() override { return false; }
	void Unmute() override {}
	void Close(bool flush) override {}
	void SetLossFactor(int factor) override {}

	std::string Statistics() const override { return "Statistics for loopback connection: N/A"; }
	std::string GetFullAddress() const override { return "Loopback"; }

private:
	std::deque< std::shared_ptr<const RawPacket> > pktQueue;
};

} // namespace netcode

#endif // _LOOPBACK_CONNECTION_H

