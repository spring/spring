#ifndef _REMOTE_CONNECTION
#define _REMOTE_CONNECTION

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
	unsigned GetSize() const
	{
		return data.size() + headerSize;
	};
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

	unsigned GetSize() const
	{
		unsigned size = headerSize + naks.size();
		for (std::list<ChunkPtr>::const_iterator it = chunks.begin(); it != chunks.end(); ++it)
			size += (*it)->GetSize();
		return size;
	};
	
	void Serialize(std::vector<uint8_t>& data);

	int32_t lastContinuous;
	int8_t nakType; // if < 0, we lost -x packets since lastContinuous, if >0, x = size of naks
	std::vector<uint8_t> naks;
	std::list<ChunkPtr> chunks;
};

/**
How Spring protocolheader looks like (size in bytes):
4 (int): number of packet (continuous)
4 (int):	last in order (tell the client we received all packages with packetNumber less or equal)
1 (unsigned char): nak (we missed x packets, starting with firstUnacked)

*/

/**
@brief Communication class over UDP
*/
class UDPConnection : public CConnection
{
public:
	UDPConnection(boost::shared_ptr<boost::asio::ip::udp::socket> NetSocket, const boost::asio::ip::udp::endpoint& MyAddr);
	UDPConnection(int sourceport, const std::string& address, const unsigned port);
	virtual ~UDPConnection();

	/**
	@brief Send packet to other instance
	*/
	virtual void SendData(boost::shared_ptr<const RawPacket> data);

	virtual bool HasIncomingData() const;

	virtual boost::shared_ptr<const RawPacket> Peek(unsigned ahead) const;

	/**
	@brief use this to recieve ready data
	@return a network message encapsulated in a RawPacket,
	or NULL if there are no more messages available.
	*/
	virtual boost::shared_ptr<const RawPacket> GetData();

	/**
	@brief update internals
	Check for unack'd packets, timeout etc.
	*/
	virtual void Update();
	
	/**
	@brief strip and parse header data and add data to waitingPackets
	UDPConnection takes the ownership of the packet and will delete it in this func
	*/
	void ProcessRawPacket(Packet& packet);

	/// send all data waiting in char outgoingData[]
	virtual void Flush(const bool forced = false);
	
	virtual bool CheckTimeout() const;
	
	virtual std::string Statistics() const;

	/// do we have these address?
	bool CheckAddress(const boost::asio::ip::udp::endpoint&) const;
	std::string GetFullAddress() const;
	
	void SetMTU(unsigned mtu);

private:
	void Init();
	
	spring_time lastChunkCreated;
	spring_time lastReceiveTime;
	spring_time lastSendTime;
	
	typedef boost::ptr_map<int,RawPacket> packetMap;
	typedef std::list< boost::shared_ptr<const RawPacket> > packetList;
	/// add header to data and send it
	void CreateChunk(const unsigned char* data, const unsigned length, const int packetNum);
	void SendIfNecessary(bool flushed);
	/// address of the other end
	boost::asio::ip::udp::endpoint addr;

	/// maximum size of packets to send
	unsigned mtu;
	
	bool sharedSocket;

	///outgoing stuff (pure data without header) waiting to be sended
	packetList outgoingData;

	/// Newly created and not yet sent
	std::deque<ChunkPtr> newChunks;
	/// packets the other side didn't ack'ed until now
	std::deque<ChunkPtr> unackedChunks;
	void AckChunks(int lastAck);
	spring_time lastUnackResent;
	/// Packets the other side missed
	void RequestResend(ChunkPtr);
	std::deque<ChunkPtr> resendRequested;
	int currentNum;

	void SendPacket(Packet& pkt);
	/// packets we have received but not yet read
	packetMap waitingPackets;
	int lastInOrder;
	int lastNak;
	spring_time lastNakTime;
	std::deque< boost::shared_ptr<const RawPacket> > msgQueue;

	/** Our socket.
	*/
	boost::shared_ptr<boost::asio::ip::udp::socket> mySocket;
	
	RawPacket* fragmentBuffer;

	// Traffic statistics and stuff //
	
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
		void DataSent(unsigned amount);
		
		float GetAverage() const;
		
	private:
		unsigned lastTime;
		unsigned trafficSinceLastTime;
		
		float average;
	};
	BandwidthUsage outgoing;
};

} //namespace netcode


#endif
