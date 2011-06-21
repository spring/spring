/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _UDP_CONNECTION_H
#define _UDP_CONNECTION_H

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/ip/udp.hpp>
#include <deque>
#include <list>

#include "Connection.h"
#include "System/myTime.h"

namespace netcode {

class Chunk
{
public:
	unsigned GetSize() const {
		return data.size() + headerSize;
	}
	static const unsigned maxSize = 254;
	static const unsigned headerSize = 5;
	int32_t chunkNumber;
	uint8_t chunkSize;
	std::vector<uint8_t> data;
};
typedef boost::shared_ptr<Chunk> ChunkPtr;

class Packet
{
public:
	static const unsigned headerSize = 5;
	Packet(const unsigned char* data, unsigned length);
	Packet(int lastContinuous, int nak);

	unsigned GetSize() const {
		unsigned size = headerSize + naks.size();
		std::list<ChunkPtr>::const_iterator chk;
		for (chk = chunks.begin(); chk != chunks.end(); ++chk) {
			size += (*chk)->GetSize();
		}
		return size;
	}

	void Serialize(std::vector<uint8_t>& data);

	int32_t lastContinuous;
	/// if < 0, we lost -x packets since lastContinuous, if >0, x = size of naks
	int8_t nakType;
	std::vector<uint8_t> naks;
	std::list<ChunkPtr> chunks;
};

/*
 * How Spring protocol-header looks like (size in bytes):
 * - 4 (int): number of the packet (continuous index)
 * - 4 (int): last in order (tell the client we received all packages with
 *   packetNumber less or equal)
 * - 1 (unsigned char): nak (we missed x packets, starting with firstUnacked)
 */

/**
 * @brief Communication class for sending and receiving over UDP
 */
class UDPConnection : public CConnection
{
public:
	UDPConnection(boost::shared_ptr<boost::asio::ip::udp::socket> netSocket,
			const boost::asio::ip::udp::endpoint& myAddr);
	UDPConnection(int sourceport, const std::string& address,
			const unsigned port);
	UDPConnection(CConnection& conn);
	virtual ~UDPConnection();

	// START overriding CConnection

	void SendData(boost::shared_ptr<const RawPacket> data);
	bool HasIncomingData() const;
	boost::shared_ptr<const RawPacket> Peek(unsigned ahead) const;
	void DeleteBufferPacketAt(unsigned index);
	boost::shared_ptr<const RawPacket> GetData();
	void Flush(const bool forced);
	bool CheckTimeout(int seconds = 0, bool initial = false) const;

	void ReconnectTo(CConnection &conn);
	bool CanReconnect() const;
	bool NeedsReconnect();

	std::string Statistics() const;
	std::string GetFullAddress() const;

	void Update();

	// END overriding CConnection


	/**
	 * @brief strip and parse header data and add data to waitingPackets
	 * UDPConnection takes the ownership of the packet and will delete it
	 * in this function.
	 */
	void ProcessRawPacket(Packet& packet);

	int GetReconnectSecs() const;

	/// Are we using this address?
	bool IsUsingAddress(const boost::asio::ip::udp::endpoint& from) const;

private:
	void InitConnection(boost::asio::ip::udp::endpoint address,
			boost::shared_ptr<boost::asio::ip::udp::socket> socket);

	void CopyConnection(UDPConnection& conn);

	void SetMTU(unsigned mtu);

	void Init();

	/// add header to data and send it
	void CreateChunk(const unsigned char* data, const unsigned length,
			const int packetNum);
	void SendIfNecessary(bool flushed);
	void AckChunks(int lastAck);

	void RequestResend(ChunkPtr);
	void SendPacket(Packet& pkt);

	spring_time lastChunkCreated;
	spring_time lastReceiveTime;
	spring_time lastSendTime;

	typedef boost::ptr_map<int,RawPacket> packetMap;
	typedef std::list< boost::shared_ptr<const RawPacket> > packetList;
	/// address of the other end
	boost::asio::ip::udp::endpoint addr;

	/// maximum size of packets to send
	unsigned mtu;

	int reconnectTime;

	bool sharedSocket;

	/// outgoing stuff (pure data without header) waiting to be sended
	packetList outgoingData;

	/// Newly created and not yet sent
	std::deque<ChunkPtr> newChunks;
	/// packets the other side did not ack'ed until now
	std::deque<ChunkPtr> unackedChunks;
	spring_time lastUnackResent;
	/// Packets the other side missed
	std::deque<ChunkPtr> resendRequested;
	int currentNum;

	/// packets we have received but not yet read
	packetMap waitingPackets;
	int lastInOrder;
	int lastNak;
	spring_time lastNakTime;
	std::deque< boost::shared_ptr<const RawPacket> > msgQueue;

	/// Our socket
	boost::shared_ptr<boost::asio::ip::udp::socket> mySocket;

	RawPacket* fragmentBuffer;

	// Traffic statistics and stuff

	/// packets that are resent
	unsigned resentChunks;
	unsigned droppedChunks;

	unsigned sentOverhead, recvOverhead;
	unsigned sentPackets, recvPackets;

	class BandwidthUsage
	{
	public:
		BandwidthUsage();
		void UpdateTime(unsigned newTime);
		void DataSent(unsigned amount, bool prel = false);

		float GetAverage(bool prel = false) const;

	private:
		unsigned lastTime;
		unsigned trafficSinceLastTime;
		unsigned prelTrafficSinceLastTime;

		float average;
	};
	BandwidthUsage outgoing;
};

} // namespace netcode

#endif // _UDP_CONNECTION_H

