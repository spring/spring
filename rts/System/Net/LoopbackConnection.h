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
	CLoopbackConnection();
	virtual ~CLoopbackConnection();

	void SendData(boost::shared_ptr<const RawPacket> data);
	bool HasIncomingData() const;
	boost::shared_ptr<const RawPacket> Peek(unsigned ahead) const;
	boost::shared_ptr<const RawPacket> GetData();
	void DeleteBufferPacketAt(unsigned index);
	void Flush(const bool forced);
	bool CheckTimeout(int seconds, bool initial) const;

	void ReconnectTo(CConnection& conn) {}
	bool CanReconnect() const;
	bool NeedsReconnect();
	void Unmute() {}
	void Close(bool flush) {}
	void SetLossFactor(int factor) {}

	std::string Statistics() const;
	std::string GetFullAddress() const;

private:
	std::deque< boost::shared_ptr<const RawPacket> > Data;
};

} // namespace netcode

#endif // _LOOPBACK_CONNECTION_H

