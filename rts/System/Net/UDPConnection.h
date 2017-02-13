/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _UDP_CONNECTION_H
#define _UDP_CONNECTION_H

#include <asio/ip/udp.hpp>
#include <map>
#include <memory>
#include <deque>
#include <list>

#include "Connection.h"
#include "System/Misc/SpringTime.h"

class CRC;


namespace netcode {

// for reliability testing, introduce fake packet loss with a percentage probability
#define NETWORK_TEST 0                        // in [0, 1] // enable network reliability testing mode
#define PACKET_LOSS_FACTOR 50                 // in [0, 100)
#define SEVERE_PACKET_LOSS_FACTOR 1           // in [0, 100)
#define PACKET_CORRUPTION_FACTOR 0            // in [0, 100)
#define SEVERE_PACKET_LOSS_MAX_COUNT 10       // max continuous number of packets to be lost
#define PACKET_MIN_LATENCY 750                // in [milliseconds] minimum latency
#define PACKET_MAX_LATENCY 1250               // in [milliseconds] maximum latency
#define ENABLE_DEBUG_STATS

class Chunk
{
public:
	unsigned GetSize() const { return (data.size() + headerSize); }
	void UpdateChecksum(CRC& crc) const;
	static const unsigned maxSize = 254;
	static const unsigned headerSize = 5;
	std::int32_t chunkNumber;
	std::uint8_t chunkSize;
	std::vector<std::uint8_t> data;
};
typedef std::shared_ptr<Chunk> ChunkPtr;

class Packet
{
public:
	static const unsigned headerSize = 6;
	Packet(const unsigned char* data, unsigned length);
	Packet(int lastContinuous, int nak);

	unsigned GetSize() const;

	std::uint8_t GetChecksum() const;

	void Serialize(std::vector<std::uint8_t>& data);

	std::int32_t lastContinuous;
	/// if < 0, we lost -x packets since lastContinuous, if >0, x = size of naks
	std::int8_t nakType;
	std::uint8_t checksum;
	std::vector<std::uint8_t> naks;
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
	UDPConnection(std::shared_ptr<asio::ip::udp::socket> netSocket,
			const asio::ip::udp::endpoint& myAddr);
	UDPConnection(int sourceport, const std::string& address,
			const unsigned port);
	UDPConnection(CConnection& conn);
	virtual ~UDPConnection();

	enum { MIN_LOSS_FACTOR = 0, MAX_LOSS_FACTOR = 2 };


	// START overriding CConnection
	void SendData(std::shared_ptr<const RawPacket> data);
	bool HasIncomingData() const { return !msgQueue.empty(); }
	std::shared_ptr<const RawPacket> Peek(unsigned ahead) const;
	std::shared_ptr<const RawPacket> GetData();
	void DeleteBufferPacketAt(unsigned index);
	void Flush(const bool forced);
	bool CheckTimeout(int seconds = 0, bool initial = false) const;

	void ReconnectTo(CConnection &conn);
	bool CanReconnect() const;
	bool NeedsReconnect();

	unsigned int GetPacketQueueSize() const { return msgQueue.size(); }

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

	int GetReconnectSecs() const { return reconnectTime; }

	/// Are we using this address?
	bool IsUsingAddress(const asio::ip::udp::endpoint& from) const;
	/// Connections are stealth by default, this allow them to send data
	void Unmute() { muted = false; }
	void Close(bool flush);
	void SetLossFactor(int factor);

	const asio::ip::udp::endpoint &GetEndpoint() const { return addr; }

private:
	void InitConnection(asio::ip::udp::endpoint address,
			std::shared_ptr<asio::ip::udp::socket> socket);

	void CopyConnection(UDPConnection& conn);

	void SetMTU(unsigned mtu);

	void Init();

	/// add header to data and send it
	void CreateChunk(const unsigned char* data, const unsigned length,
			const int packetNum);
	void SendIfNecessary(bool flushed);
	void AckChunks(int lastAck);

	void RequestResend(ChunkPtr ptr);
	void SendPacket(Packet& pkt);

	spring_time lastChunkCreatedTime;
	spring_time lastPacketSendTime;
	spring_time lastPacketRecvTime;

	spring_time lastUnackResentTime;
	spring_time lastNakTime;
	#ifdef ENABLE_DEBUG_STATS
	spring_time lastDebugMessageTime;
	spring_time lastFramePacketRecvTime;
	#endif

	typedef std::map<int,RawPacket*> packetMap;
	typedef std::list< std::shared_ptr<const RawPacket> > packetList;
	/// address of the other end
	asio::ip::udp::endpoint addr;

	/// maximum size of packets to send
	unsigned mtu;

	bool muted;
	bool closed;
	bool resend;
	bool sharedSocket;
	bool logMessages;

	int netLossFactor;
	int reconnectTime;

	/// outgoing stuff (pure data without header) waiting to be sent
	packetList outgoingData;
	/// packets we have received but not yet read
	packetMap waitingPackets;

	/// Newly created and not yet sent
	std::deque<ChunkPtr> newChunks;
	/// packets the other side did not ack'ed until now
	std::deque<ChunkPtr> unackedChunks;

	/// Packets the other side missed
	std::map<std::int32_t, ChunkPtr> resendRequested;

	/// complete packets we received but did not yet consume
	std::deque< std::shared_ptr<const RawPacket> > msgQueue;

	std::int32_t lastMidChunk;

#if	NETWORK_TEST
	/// Delayed packets, for testing purposes
	std::map< spring_time, std::vector<std::uint8_t> > delayed;
	int lossCounter;
#endif

	int lastInOrder;
	int lastNak;

	/// Our socket
	std::shared_ptr<asio::ip::udp::socket> mySocket;

	RawPacket* fragmentBuffer;

	// Traffic statistics and stuff
	#ifdef ENABLE_DEBUG_STATS
	float sumDeltaFramePacketRecvTime;
	float minDeltaFramePacketRecvTime;
	float maxDeltaFramePacketRecvTime;

	unsigned int numReceivedFramePackets;
	unsigned int numEnqueuedFramePackets;
	unsigned int numEmptyGetDataCalls;
	unsigned int numTotalGetDataCalls;
	#endif
	unsigned int currentPacketChunkNum;

	/// packets that are resent
	unsigned int resentChunks;
	unsigned int droppedChunks;

	unsigned int sentOverhead, recvOverhead;
	unsigned int sentPackets, recvPackets;

	class BandwidthUsage {
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

