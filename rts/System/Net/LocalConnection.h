/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LOCAL_CONNECTION_H
#define _LOCAL_CONNECTION_H

#include <boost/thread/mutex.hpp>
#include <deque>

#include "Connection.h"

namespace netcode {

/**
 * @brief Class for local connection between server / client
 * Directly connects to each others inputbuffer to increase performance.
 * The server and the client have to run in one instance of spring
 * for this to work, otherwise a normal UDP connection had to be used.
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

	/**
	 * @brief Send packet to other instance
	 *
	 * Use this, since it does not need memcpy'ing
	 */
	virtual void SendData(boost::shared_ptr<const RawPacket> data);

	virtual boost::shared_ptr<const RawPacket> Peek(unsigned ahead) const;

	/**
	 * @brief Deletes a packet from the buffer
	 * @param queue index number
	 * useful for messages that skips queuing and needs to be processed immediately
	 */
	void DeleteBufferPacketAt(unsigned index);

	/**
	 * @brief Get data
	 */
	virtual boost::shared_ptr<const RawPacket> GetData();

	/// does nothing
	virtual void Flush(const bool forced = false);

	/// is always false
	virtual bool CheckTimeout(int nsecs = 0, bool initial = false) const;

	virtual void ReconnectTo(CConnection& conn) {}
	virtual bool CanReconnect() const;
	virtual bool NeedsReconnect();

	virtual std::string Statistics() const;
	virtual std::string GetFullAddress() const {
		return "shared memory";
	}
	virtual bool HasIncomingData() const;

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

