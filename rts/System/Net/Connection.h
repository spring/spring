/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CONNECTION_H
#define _CONNECTION_H

#include <string>
#include <memory>

#include "RawPacket.h"

namespace netcode
{

/**
 * @brief Base class for connecting to various recievers / senders
 */
class CConnection
{
public:
	virtual ~CConnection() {}

	/**
	 * @brief Send packet to other instance
	 *
	 * Use this, since it does not need memcpy'ing
	 */
	virtual void SendData(std::shared_ptr<const RawPacket> data) = 0;

	virtual bool HasIncomingData() const = 0;

	/**
	 * @brief Take a look at the messages that will be returned by GetData().
	 * @return A RawPacket holding the data, or 0 if no data
	 * @param ahead How many packets to look ahead. A typical usage would be:
	 *   for (int ahead = 0; (packet = conn->Peek(ahead)); ++ahead) {}
	 */
	virtual std::shared_ptr<const RawPacket> Peek(unsigned ahead) const = 0;

	/**
	 * @brief use this to recieve ready data
	 * @return a network message encapsulated in a RawPacket,
	 *   or NULL if there are no more messages available.
	 */
	virtual std::shared_ptr<const RawPacket> GetData() = 0;

	/**
	 * @brief Deletes a packet from the buffer
	 * @param index queue index number
	 * Useful for messages that skip queuing and need to be processed
	 * immediately.
	 */
	virtual void DeleteBufferPacketAt(unsigned index) = 0;

	/**
	 * Flushes the underlying buffer (to the network), if there is a buffer.
	 */
	virtual void Flush(const bool forced = false) = 0;
	virtual bool CheckTimeout(int seconds = 0, bool initial = false) const = 0;

	virtual void ReconnectTo(CConnection& conn) = 0;
	virtual bool CanReconnect() const = 0;
	virtual bool NeedsReconnect() = 0;

	unsigned int GetDataReceived() const { return dataRecv; }
	unsigned int GetNumQueuedPings() const { return numPings; }
	virtual unsigned int GetPacketQueueSize() const { return 0; }

	virtual std::string Statistics() const = 0;
	virtual std::string GetFullAddress() const = 0;
	virtual void Unmute() = 0;
	virtual void Close(bool flush = false) = 0;
	virtual void SetLossFactor(int factor) = 0;

	/**
	 * @brief update internals
	 * Check for unack'd packets, timeout etc.
	 */
	virtual void Update() {}

protected:
	unsigned int dataSent = 0;
	unsigned int dataRecv = 0;
	unsigned int numPings = 0;
};

} // namespace netcode

#endif // _CONNECTION_H

