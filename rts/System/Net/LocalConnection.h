/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LOCAL_CONNECTION_H
#define _LOCAL_CONNECTION_H

#include <boost/thread/mutex.hpp>
#include <deque>

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

	void SendData(boost::shared_ptr<const RawPacket> data);
	bool HasIncomingData() const;
	boost::shared_ptr<const RawPacket> Peek(unsigned ahead) const;
	void DeleteBufferPacketAt(unsigned index);
	boost::shared_ptr<const RawPacket> GetData();
	void Flush(const bool forced);
	bool CheckTimeout(int seconds, bool initial) const;

	void ReconnectTo(CConnection& conn) {}
	bool CanReconnect() const;
	bool NeedsReconnect();

	std::string Statistics() const;
	std::string GetFullAddress() const;

	// END overriding CConnection

private:
	static std::deque< boost::shared_ptr<const RawPacket> > Data[2];
	static boost::mutex Mutex[2];

	unsigned OtherInstance() const;

	/// we can have 2 instances, one in serverNet and one in net
	static unsigned instances;
	/// which instance we are
	unsigned instance;
};

} // namespace netcode

#endif // _LOCAL_CONNECTION_H

